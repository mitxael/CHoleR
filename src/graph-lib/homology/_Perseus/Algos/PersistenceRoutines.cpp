/*
 * PersistenceRoutines.hpp
 *
 * Contains persistence algorithm over fields; originally by
 * Zomorodian and Carlsson
 */

#include "../Complexes/PComplex.h"
#include <sstream>
#include <string>

// Helper Routine for Compute_Intervals, ZC
template <typename C, typename BT>
void PComplex<C,BT>::REMOVE_PIVOT_ROW(const PCell<C,BT>* cell, PCCHAIN& delta, bool trace=false)
{
	delta.clear();
	PCCHAIN toAdd;

	PCell<C,BT>* max = NULL;

	delta = cell->getBD();
	if(RPRTALK || trace)
	{
		std::cout<<"\n    RPR delta: "<<delta;
		std::cin.get();
	}

	C q=0, p=0;
	//num i;

	delta.removeUnmarked(trace);
	if (RPRTALK || trace)
	{
		std::cout<<"\n    delta unm: "<<delta;
		std::cin.get();
	}

	num counter = 0; // loop iterations.

	while(delta.size() != 0)
	{
		counter++;
		//if (counter > 1000) trace = true;
		// optionally, can set some num i = this to get the index
		delta.getMaxIndex(max);

		if (max->TLIST->size()==0)
		{
			if (RPRTALK || trace)
			{
				std::cout<<"\n      TLIST of max cell "<<*max<<" empty, ditching!";
				std::cin.get();
			}
			break;
		}

		q = max->TLIST->getCoeff(max);

		toAdd = *(max->TLIST);
		if (RPRTALK || trace)
		{
			std::cout<<"\n    toAdd before: "<<toAdd;
			std::cin.get();
		}
		p = delta.getCoeff(max);

		if (RPRTALK || trace)
		{
			std::cout<<"\n       "<<q<<" T-coeff of "<<*max<<" vs "<<p<<" d-coeff";
			std::cout<<"\n     toAdd: "<<toAdd;
			std::cin.get();
		}

		// we should check that q is a unit here!!
		toAdd.scaleMe(-p/q);
		toAdd.makeZp(2); // enforce Z_2 coeffici
		delta += toAdd;
		delta.makeZp(2);

		if (RPRTALK || trace)
		{
			std::cout<<"        scaled toadd: "<<toAdd;
			std::cout<<"\n    ****<<loop-end>> delta: "<<delta;
		}
	}
}

// compute persistence intervals
template <typename C, typename BT>
void PComplex<C,BT>::COMPUTE_INTERVALS(const Complex<C,BT>& other, bool makegens = false, bool truncate = false)
{
	// initialize betti and interval structures!
	initPersData(other);

	// order cells linearly by birth and dimension, in that order
	num n = makeFromComplex(other, truncate);

	if (PERSTALK || FLOWTALK) std::cout<<"\nLinearly ordered "<<n<<" cells...";
	if (MAKEBPS) {std::cout<<"\n  ... ";    std::cin.get(); }

	PCCHAIN delta;

	// variables for indices into the linear list
	num curmaxind, curdim, maxdim;

	BT curbirth, curdeath;
	PCell<C,BT>* max;
	PCell<C,BT>* cur;


	bool trace;
	// now the routine, loop over linear list of cells,...
	typename PCVEC::const_iterator curcell;
	for (curcell = klist.begin(); curcell != klist.end(); ++curcell)
	{
		trace = false;
		cur = *curcell;

		if (cur == NULL) std::cout<<" NULL ALERT!!!";
		//if (cur->isMarked()) cout<<"\n random marked alert on "<<*cur;

		//trace = true;
/*		// insert traces here
		if (cur->kindex % 1000 == 0)
		{
			cout<<"\n trace: "<<cur; cin.get();
			trace = true;
		}
*/
		if (PERSTALK || trace)
		{
			std::cout<<"\n address: "<<cur; std::cin.get();
			std::cout<<"\nHandling Cell: "<<*cur;
			std::cout<<"\nwith boundary: "<<cur->getBD();
		}
		// remove its pivot row, store resulting chain delta
		REMOVE_PIVOT_ROW(cur,delta,trace);

		if (PERSTALK || trace) std::cout<<"\nReturned delta: "<<delta;

		// mark if delta is trivial
		if (delta.size()==0)
		{
			cur->mark();
			if (PERSTALK || trace) std::cout<<"      marked: "<<*cur<<"@"<<cur;
		}
		else
		{
			//cur->marked = false;
			curdim = (*curcell)->getDim();
			curmaxind = delta.getMaxIndex(max);

			max = klist.at(curmaxind);
			if(PERSTALK || trace) std::cout<<"\n     killed: "<<*max;
			maxdim = max->getDim();

			*(max->TLIST) = delta;
			if (trace)
			{
				std::cout<<"\n********update tlist of max cell "<<*max<<" to "<<delta<<" killed by "<<*cur;
			}
			curbirth = max->birth;
			curdeath = cur->birth;

			// this is where we should store max's king chain as "generator info?

			if (curbirth < curdeath)
			{
				//cout<<"\n interval: "<<*cur<<" "<<*max;
				ints[maxdim]->push_back(std::make_pair(curbirth, curdeath));
				incrementBetti(curbirth, curdeath, maxdim);
			}
		}
	}

	//cout<<"escape1"; cin.get();

	for (curcell = klist.begin(); curcell != klist.end(); ++curcell)
	{
		// extract current cell
		cur = *curcell;
		curdim = cur->getDim(); // and its dimension
		if (cur->isMarked()) // if this is marked...
		{
			curbirth = cur->birth;
			// if this cell is never killed, then note down the
			// corresponding interval (death at -1) is stored...
			if (cur->TLIST->size()==0)
			{
			   curdeath = BANBT;
			   //cout<<"\nforever interval: "<<*cur;
			   //cout<<"\n cbd: "<<cur->getCB();
			  // cout<<"\n whose bd is "<<cur->getCB().getBD();

			   //if (makegens && cur->kgen != NULL) genfile << curdim <<" "<< curbirth <<" "<< curdeath <<" "<< *(cur->kgen) <<"\n";

			   ints[curdim]->push_back(std::make_pair(curbirth,curdeath));
			   incrementBetti(curbirth,curdeath,curdim);
			}
		}
	}
}

// increments the dim'th betti number for all frames from
// "from" to "to", returns number incremented
template <typename C, typename BT>
num PComplex<C,BT>::incrementBetti(const BT& from, const BT& to, const num& dim)
{
	num toret = 0;
	typename BETTI_STR::iterator curpos;
	typename BETTI_STR::iterator halt = betti.find(to);

	for(curpos = betti.find(from); curpos != betti.end(); ++curpos)
	{
		if (curpos==halt) break;
		toret++;
		if (dim < (num)curpos->second->size()) curpos->second->at(dim)++;
		//else cout<<"dimension madness!";
	}
	return toret;
}