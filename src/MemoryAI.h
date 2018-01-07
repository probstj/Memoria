#ifndef MEMORYAI_H
#define MEMORYAI_H

#include <QObject>  
#include <stdio.h>  // for printf()
#include <map>
#include <vector>
#include "MTriple.cpp"

using namespace std;

struct Position_t { 
    unsigned int _column, _row; 
    Position_t( unsigned int column, unsigned int row) : _column( column), _row( row) {};
};

class MemoryAI : public QObject
{
    Q_OBJECT

private:
    enum MEMORY_STATUS
    {
        KNOWS_PAIR = 0,
        RANDOM_GUESS = 1
    };

    unsigned int _numOfColumns;
    unsigned int _numOfRows;
    unsigned int _numOfTiles;
    unsigned int _difficulty;
    double _difficultyRate;
    bool _isFirstTile;
    bool _useKnownPairs;
    MTriple _firstTile;
    Position_t* _firstGuess;
    MEMORY_STATUS _status;
    bool _verbose;

    // all tiles that have not been removed yet, i.e. are available to turn over:
    vector< Position_t*> _availableTiles;
    // these tiles have not been turned over yet, i.e. have unknown ids:
    vector< Position_t*> _unknownTiles; 
    // these tiles have been seen (i.e. their ids are known) but not their matching second tile to make a pair:
    map< unsigned int, Position_t> _seenTiles; 
    // the _seenTiles with a known partner are moved to this map, where all known pairs are kept:
    map< unsigned int, pair< Position_t, Position_t> > _knownPairs; 
    // TODO: Die _id der MTriple in _seenTiles und _knownPairs wird niemals verwendet. 
    // Beide maps koennten also auch Position_t anstatt MTriple verwenden, um Speicher zu sparen.
    // MTriple._id wird nur benoetigt bei _firstTile, auch nicht bei randomGuess oder _firstGuess;
    

    void memoriseTile( unsigned int id, unsigned int column, unsigned int row);
    void dememoriseTiles( unsigned int id, unsigned int column1, unsigned int row1, unsigned int column2, unsigned int row2);

    Position_t* randomGuess() const;
    // if this is true, prints out what is happening to stdout:

    // kein Kopieren
    MemoryAI( const MemoryAI&);
    // kein Zuweisen
    MemoryAI& operator=( const MemoryAI&);

public:
    MemoryAI( unsigned int columns, unsigned int rows, unsigned int numOfTiles, unsigned int difficulty, QObject* parent = 0);
    ~MemoryAI();

    //void guess();

    void firstGuess();
    void secondGuess();
    
    // call this to print out what is happening to stdout:
    void setVerbosity(bool enabled = true);
        
public slots:
    void revealedTile( unsigned int id, unsigned int column, unsigned int row);
    
signals:
    void submitGuess(uint column, uint row);

};

#endif