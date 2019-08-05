/*
 * PComplex.h
 * Contains Morse Complex Class Definition
 */


#ifndef COMPLEX_H_
#define COMPLEX_H_

# include <list>
# include <deque>
# include <algorithm>
# include <map>

# include "../DebugV.h"
# include "../Cells/Cell.h"


#define CLIST std::vector<Cell<C,BT>*>
#define FRAME std::map<num, CLIST*>
#define COMPLEX std::map<BT, FRAME*>

#define DSIZE std::map<num, std::pair<num,num> >
#define DMAP std::map<BT, DSIZE*>

 // C is the class of Ring Coefficients, BT is the space
 // of birth times for cells
template <typename C, typename BT = int>
class Complex
{
public:
	// generic cell complex data
	COMPLEX clist; // Cell pointers arranged by birth + dimension + index
	// so clist[b][d] is a list containing cells born at b of dimension d

	DMAP sizeinfo; // maps each birth time to DSIZE map; a DSIZE map maps
	// each dimension to a pair, the first number being the total number of
	// cells of that (birth and) dimension and the second being the largest
	// index of a cell of that dimension.

	// constructor
	Complex()
	{
		// cout<<"Complex called with: "<<dim;
		Init();
	}

	void Init()
	{
		clist.clear();
		sizeinfo.clear();
		//remcells.clear();
	}

	num lsize(const BT, const num) const; // size of list for given frame + dim
	num dsize(const num, BT) const; // how many cells of this dimension?
	num fsize(const BT) const; // how many cells born at this time?
	num size() const; // size of entire complex!

	void squeeze(); //compresses cell vectors to save memory
	std::pair<num, num> getSizeInfo(const BT&, const num&) const;

	// deletes all allocated memory, optionally deletes cells too
	void Destroy(bool);

	virtual ~Complex()
	{
		///Destroy();
	}

	// inserts a cell into the complex; should have birth & dim already
	bool insertCell(Cell<C, BT>* toin);

	// faster insert, entire vector
	void quicksert(const BT, const num, const std::vector<Cell<C, BT>*>&);

	// removes the cell in question from the complex
	// rc removes cell from complex, fmb from bdrys of higher cells,
	// fmc from cobdrys of lower cells
	bool removeCell(Cell<C, BT>*, typename CLIST::iterator&, bool, bool, bool);

	// search: returns iterator to cell in complex, if any
	bool isIn(const Cell<C, BT>* tofind, typename CLIST::iterator& pos);

	// resets the top dimension for given frame, checking clist for emptiness
	std::pair<num, bool> getTopDim(const BT&) const;
	std::pair<num, bool> getTopDim() const; // returns overall max dim

	// resets bottom dimension, checking clist for emptiness
	std::pair<num, bool> getBotDim(const BT&) const;
	std::pair<num, bool> getBotDim() const; // returns overall min dim

	// prints the complex... don't try this at home with large complexes
	friend std::ostream& operator << (std::ostream& out, const Complex<C, BT>& toprint)
	{
		toprint.print(out);
		return out;
	}


	void print(std::ostream&) const;
	void printFrame(const BT& birth, std::ostream&) const;
	void printList(const BT& birth, const num dim, std::ostream&) const;
	void printSizeInfo(bool, bool, std::ostream&) const;
	void printSizeStruct(std::ostream&) const;
	num pruneList(const BT&, const num);
	bool checkComplex(std::ostream&) const; // checks del-square of every cell to make sure we get 0
	void clear();
	void clearDim(num dim);


	//bool suspend(num dim); // suspension
};

#endif /* COMPLEX_H_ */
