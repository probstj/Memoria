#ifndef MTRIPLE_H
#define MTRIPLE_H

class MTriple
{
public:
	unsigned int _id;
	unsigned int _column;
	unsigned int _row;

	MTriple( unsigned int id, unsigned int column, unsigned int row): _id( id), _column( column), _row( row) {}
	~MTriple() {}
};


#endif