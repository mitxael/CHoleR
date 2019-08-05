﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using QuickGraph;

namespace GraphSharp.Algorithms.Layout.Compound.FDP
{
    public partial class CompoundFDPLayoutAlgorithm<TVertex, TEdge, TGraph>
        where TVertex : class
        where TEdge : IEdge<TVertex>
        where TGraph : IBidirectionalGraph<TVertex, TEdge>
    {
        /// <summary>
        /// Informations for compound vertices.
        /// </summary>
        private readonly IDictionary<TVertex, CompoundVertexData> _compoundVertexDatas =
            new Dictionary<TVertex, CompoundVertexData>();

        /// <summary>
        /// Informations for the simple vertices.
        /// </summary>
        private readonly IDictionary<TVertex, SimpleVertexData> _simpleVertexDatas =
            new Dictionary<TVertex, SimpleVertexData>();

        /// <summary>
        /// Informations for all kind of vertices.
        /// </summary>
        private readonly IDictionary<TVertex, VertexData> _vertexDatas =
            new Dictionary<TVertex, VertexData>();

        /// <summary>
        /// The levels of the graph (generated by the containment associations).
        /// </summary>
        private readonly IList<HashSet<TVertex>> _levels =
            new List<HashSet<TVertex>>();

        private class RemovedTreeNodeData<TVertex, TEdge>
        {
            public readonly TVertex Vertex;
            public readonly TEdge Edge;

            public RemovedTreeNodeData(TVertex vertex, TEdge edge)
            {
                Vertex = vertex;
                Edge = edge;
            }
        }

        /// <summary>
        /// The list of the removed root-tree-nodes and edges by it's level
        /// (level = distance from the closest not removed node).
        /// </summary>
        private readonly Stack<IList<RemovedTreeNodeData<TVertex,TEdge>>> _removedRootTreeNodeLevels =
            new Stack<IList<RemovedTreeNodeData<TVertex,TEdge>>>();

        private readonly HashSet<TVertex> _removedRootTreeNodes = new HashSet<TVertex>();

        private readonly HashSet<TEdge> _removedRootTreeEdges = new HashSet<TEdge>();

        /// <summary>
        /// The dictionary of the initial vertex sizes.
        /// DO NOT USE IT AFTER THE INITIALIZATION.
        /// </summary>
        private readonly IDictionary<TVertex, Size> _vertexSizes;

        /// <summary>
        /// The dictionary of the vertex bordex.
        /// DO NOT USE IT AFTER THE INITIALIZATION.
        /// </summary>
        private readonly IDictionary<TVertex, Thickness> _vertexBorders;

        /// <summary>
        /// The dictionary of the layout types of the compound vertices.
        /// DO NOT USE IT AFTER THE INITIALIZATION.
        /// </summary>
        private readonly IDictionary<TVertex, CompoundVertexInnerLayoutType> _layoutTypes;

        private readonly IMutableCompoundGraph<TVertex, TEdge> _compoundGraph;

        /// <summary>
        /// Represents the root vertex.
        /// </summary>
        private readonly CompoundVertexData _rootCompoundVertex =
            new CompoundVertexData(
                null, null, false, new Point(),
                new Size(), new Thickness(),
                CompoundVertexInnerLayoutType.Automatic);

        public CompoundFDPLayoutAlgorithm(
            TGraph visitedGraph,
            IDictionary<TVertex, Size> vertexSizes,
            IDictionary<TVertex, Thickness> vertexBorders,
            IDictionary<TVertex, CompoundVertexInnerLayoutType> layoutTypes,
            IDictionary<TVertex, Point> vertexPositions,
            CompoundFDPLayoutParameters oldParameters)
            : base(visitedGraph, vertexPositions, oldParameters)
        {
            _vertexSizes = vertexSizes;
            _vertexBorders = vertexBorders;
            _layoutTypes = layoutTypes;

            if (VisitedGraph is ICompoundGraph<TVertex, TEdge>)
                _compoundGraph = new CompoundGraph<TVertex, TEdge>(VisitedGraph as ICompoundGraph<TVertex, TEdge>);
            else
                _compoundGraph = new CompoundGraph<TVertex, TEdge>(VisitedGraph);
        }

        #region ICompoundLayoutAlgorithm<TVertex,TEdge,TGraph> Members

        public IDictionary<TVertex, Size> InnerCanvasSizes
        {
            get
            {
                return _compoundVertexDatas.ToDictionary(
                    kvp => kvp.Key,
                    kvp => kvp.Value.InnerCanvasSize);
            }
        }

        #endregion

        #region Nested type: CompoundVertexData

        /// <summary>
        /// Data for the compound vertices.
        /// </summary>
        private class CompoundVertexData : VertexData
        {
            /// <summary>
            /// The thickness of the borders of the compound vertex.
            /// </summary>
            public readonly Thickness Borders;

            /// <summary>
            /// Gets the layout type of the compound vertex.
            /// </summary>
            public readonly CompoundVertexInnerLayoutType InnerVertexLayoutType;

            private Size _innerCanvasSize;

            private Size _size;

            private ICollection<VertexData> _children;

            public CompoundVertexData(TVertex vertex,
                                      VertexData movableParent,
                                      bool isFixedToParent,
                                      Point position,
                                      Size size,
                                      Thickness borders,
                                      CompoundVertexInnerLayoutType innerVertexLayoutType)
                : base(vertex, movableParent, isFixedToParent, position)
            {
                Borders = borders;

                //calculate the size of the inner canvas
                InnerCanvasSize = new Size(Math.Max(0.0, size.Width - Borders.Left - Borders.Right),
                                           Math.Max(0.0, size.Height - Borders.Top - Borders.Bottom));
                InnerVertexLayoutType = innerVertexLayoutType;
            }

            /// <summary>
            /// The size of the inner canvas of the compound vertex.
            /// </summary>
            public Size InnerCanvasSize
            {
                get { return _innerCanvasSize; }
                set
                {
                    _innerCanvasSize = value;

                    //set the size of the canvas
                    _size = new Size(_innerCanvasSize.Width + Borders.Left + Borders.Right,
                                     _innerCanvasSize.Height + Borders.Top + Borders.Bottom);
                }
            }

            /// <summary>
            /// The overall size of the vertex (inner canvas size + borders + ...).
            /// </summary>
            public override Size Size
            {
                get { return _size; }
            }

            /// <summary>
            /// Modifies the position of the children with the given
            /// vector.
            /// </summary>
            /// <param name="force">The vector of the position modification.</param>
            private void PropogateToChildren(Vector force)
            {
                foreach (var child in _children)
                {
                    child.ApplyForce(force);
                }
            }

            public ICollection<VertexData> Children
            {
                get { return _children; }
                set
                {
                    _children = value;
                }
            }

            internal override void ApplyForce(Vector force)
            {
                Position += force;
                PropogateToChildren(force);
                RecalculateBounds();
            }

            public Point InnerCanvasCenter
            {
                get
                {
                    return new Point(
                        Position.X - Size.Width / 2 + Borders.Left + InnerCanvasSize.Width / 2,
                        Position.Y - Size.Height / 2 + Borders.Top + InnerCanvasSize.Height / 2
                        );
                }
                set
                {
                    Position = new Point(
                        value.X - InnerCanvasSize.Width / 2 - Borders.Left + Size.Width / 2,
                        value.Y - InnerCanvasSize.Height / 2 - Borders.Top + Size.Height / 2
                        );
                }
            }

            public void RecalculateBounds()
            {
                if (_children == null)
                {
                    InnerCanvasSize = new Size(); //TODO padding
                    return;
                }

                Point topLeft = new Point(double.PositiveInfinity, double.PositiveInfinity);
                Point bottomRight = new Point(double.NegativeInfinity, double.NegativeInfinity);
                foreach (var child in _children)
                {
                    topLeft.X = Math.Min(topLeft.X, child.Position.X - child.Size.Width / 2);
                    topLeft.Y = Math.Min(topLeft.Y, child.Position.Y - child.Size.Height / 2);

                    bottomRight.X = Math.Max(bottomRight.X, child.Position.X + child.Size.Width / 2);
                    bottomRight.Y = Math.Max(bottomRight.Y, child.Position.Y + child.Size.Height / 2);
                }
                InnerCanvasSize = new Size(bottomRight.X - topLeft.X, bottomRight.Y - topLeft.Y);
                InnerCanvasCenter = new Point((topLeft.X + bottomRight.X) / 2.0, (topLeft.Y + bottomRight.Y) / 2.0);
            }
        }

        #endregion

        #region Nested type: SimpleVertexData

        private class SimpleVertexData : VertexData
        {
            /// <summary>
            /// The size of the vertex.
            /// </summary>
            private readonly Size _size;

            public SimpleVertexData(TVertex vertex, VertexData movableParent, bool isFixed, Point position, Size size)
                : base(vertex, movableParent, isFixed, position)
            {
                _size = size;
            }

            /// <summary>
            /// Gets the actual size of the vertex (inner size + border + anything else...).
            /// </summary>
            public override Size Size
            {
                get { return _size; }
            }

            internal override void ApplyForce(Vector force)
            {
                Position += force;
            }
        }

        #endregion

        #region Nested type: VertexData

        /// <summary>
        /// Data for the simple vertices.
        /// </summary>
        private abstract class VertexData
        {
            /// <summary>
            /// Gets the vertex which is wrapped by this object.
            /// </summary>
            public readonly TVertex Vertex;
            public CompoundVertexData Parent;
            private Vector _springForce;
            private Vector _repulsionForce;
            private Vector _gravitationForce;
            private Vector _applicationForce;
            private Vector _previousForce;
            private Vector _childrenForce;

            protected VertexData(TVertex vertex, VertexData movableParent, bool isFixedToParent, Point position)
            {
                Vertex = vertex;
                MovableParent = movableParent;
                IsFixedToParent = isFixedToParent;
                Parent = null;
                Position = position;
            }

            /// <summary>
            /// If the vertex is fixed (cannot be moved), that's it's parent
            /// that could be moved (if there's any).
            /// 
            /// This property can only be set once.
            /// </summary>
            public VertexData MovableParent { get; set; }

            /// <summary>
            /// Gets or sets that the position of the vertex is fixed to
            /// it's parent vertex or not.
            /// </summary>
            public bool IsFixedToParent { get; set; }

            /// <summary>
            /// Gets the actual size of the vertex (inner size + border + anything else...).
            /// </summary>
            public abstract Size Size { get; }

            /// <summary>
            /// The level of the vertex inside the graph.
            /// </summary>
            public int Level;

            /// <summary>
            /// The position of the vertex.
            /// </summary>
            public Point Position;

            /// <summary>
            /// Gets or sets the spring force.
            /// </summary>
            public Vector SpringForce
            {
                get { return IsFixedToParent ? new Vector() : _springForce; }
                set
                {
                    if (IsFixedToParent)
                        _springForce = new Vector();
                    else _springForce = value;
                }
            }

            /// <summary>
            /// Gets or sets the spring force.
            /// </summary>
            public Vector RepulsionForce
            {
                get { return IsFixedToParent ? new Vector() : _repulsionForce; }
                set
                {
                    if (IsFixedToParent)
                        _repulsionForce = new Vector();
                    else _repulsionForce = value;
                }
            }

            /// <summary>
            /// Gets or sets the spring force.
            /// </summary>
            public Vector GravitationForce
            {
                get { return IsFixedToParent ? new Vector() : _gravitationForce; }
                set
                {
                    if (IsFixedToParent)
                        _gravitationForce = new Vector();
                    else _gravitationForce = value;
                }
            }

            /// <summary>
            /// Gets or sets the spring force.
            /// </summary>
            public Vector ApplicationForce
            {
                get { return IsFixedToParent ? new Vector() : _applicationForce; }
                set
                {
                    if (IsFixedToParent)
                        _applicationForce = new Vector();
                    else _applicationForce = value;
                }
            }

            internal abstract void ApplyForce(Vector force);
            public Vector ApplyForce(double limit)
            {
                var force = _springForce
                    + _repulsionForce
                    + _gravitationForce
                    + _applicationForce
                    + 0.5 * _childrenForce;

                Parent._childrenForce += force;

                if (force.Length > limit)
                    force *= (limit / force.Length);
                force += 0.7 * _previousForce;
                if (force.Length > limit)
                    force *= (limit / force.Length);

                ApplyForce(force);
                _springForce = new Vector();
                _repulsionForce = new Vector();
                _gravitationForce = new Vector();
                _applicationForce = new Vector();
                _childrenForce = new Vector();

                _previousForce = force;
                return force;
            }
        }

        #endregion
    }
}
