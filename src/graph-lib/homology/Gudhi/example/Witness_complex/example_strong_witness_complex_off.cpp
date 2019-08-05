/*    This file is part of the Gudhi Library. The Gudhi library
 *    (Geometric Understanding in Higher Dimensions) is a generic C++
 *    library for computational topology.
 *
 *    Author(s):       Siargey Kachanovich
 *
 *    Copyright (C) 2016  INRIA (France)
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gudhi/Simplex_tree.h>
#include <gudhi/Euclidean_strong_witness_complex.h>
#include <gudhi/pick_n_random_points.h>
#include <gudhi/choose_n_farthest_points.h>
#include <gudhi/Points_off_io.h>

#include <CGAL/Epick_d.h>

#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <vector>

using K = CGAL::Epick_d<CGAL::Dynamic_dimension_tag>;
using Point_d = typename K::Point_d;
using Witness_complex = Gudhi::witness_complex::Euclidean_strong_witness_complex<K>;
using Point_vector = std::vector<Point_d>;

int main(int argc, char* const argv[]) {
  if (argc != 5) {
    std::cerr << "Usage: " << argv[0] << " path_to_point_file number_of_landmarks max_squared_alpha limit_dimension\n";
    return 0;
  }

  std::string file_name = argv[1];
  int nbL = atoi(argv[2]), lim_dim = atoi(argv[4]);
  double alpha2 = atof(argv[3]);
  clock_t start, end;
  Gudhi::Simplex_tree<> simplex_tree;

  // Read the point file
  Point_vector point_vector, landmarks;
  Gudhi::Points_off_reader<Point_d> off_reader(file_name);
  if (!off_reader.is_valid()) {
    std::cerr << "Strong witness complex - Unable to read file " << file_name << "\n";
    exit(-1);  // ----- >>
  }
  point_vector = Point_vector(off_reader.get_point_cloud());

  std::cout << "Successfully read " << point_vector.size() << " points.\n";
  std::cout << "Ambient dimension is " << point_vector[0].dimension() << ".\n";

  // Choose landmarks (decomment one of the following two lines)
  // Gudhi::subsampling::pick_n_random_points(point_vector, nbL, std::back_inserter(landmarks));
  Gudhi::subsampling::choose_n_farthest_points(K(), point_vector, nbL, Gudhi::subsampling::random_starting_point,
                                               std::back_inserter(landmarks));

  // Compute witness complex
  start = clock();
  Witness_complex witness_complex(landmarks, point_vector);

  witness_complex.create_complex(simplex_tree, alpha2, lim_dim);
  end = clock();
  std::cout << "Strong witness complex took " << static_cast<double>(end - start) / CLOCKS_PER_SEC << " s. \n";
  std::cout << "Number of simplices is: " << simplex_tree.num_simplices() << "\n";
}
