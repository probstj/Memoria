/*
 * MIT License
 *
 * Copyright (c) 2014 Semih Özen, Jürgen Probst
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "MemoryAI.h"
//#include <time.h>

MemoryAI::MemoryAI( unsigned int columns, unsigned int rows, unsigned int numOfTiles, unsigned int difficulty, QObject* parent):
    QObject(parent), _numOfColumns( columns), _numOfRows( rows), _numOfTiles( numOfTiles),
    _difficulty( difficulty), _difficultyRate( 1.),
    _isFirstTile( true), _useKnownPairs( false), _firstTile( 0, 0 ,0), _firstGuess( NULL),
    _status( KNOWS_PAIR), _verbose(false)


{
	//srand ( (unsigned int) time(NULL)); // already done in Memory constructor
	if( _difficulty < 1 || 4 < _difficulty)
		_difficulty = 2;
	_difficultyRate = _difficulty/4.;

	_availableTiles.reserve( numOfTiles);
        _unknownTiles.reserve( numOfTiles);
	for( unsigned int r = 0 ; r < rows ; ++r)
		for( unsigned int c = 0 ; c < columns ; ++c, --numOfTiles)
		{
			if( 0 == numOfTiles)
				break;
                        Position_t* pt = new Position_t( c, r);
                        _availableTiles.push_back( pt);
                        _unknownTiles.push_back( pt);
		}
}

MemoryAI::~MemoryAI()
{
    for( unsigned int i = 0 ; i < _unknownTiles.size() ; ++i)
    {
        _unknownTiles[i] = NULL;
    }
    for( unsigned int i = 0 ; i < _availableTiles.size() ; ++i)
    {
        delete _availableTiles[i];
        _availableTiles[i] = NULL;
    }
}


void MemoryAI::memoriseTile( unsigned int id, unsigned int column, unsigned int row)
{
    if (_verbose)
        printf("AI: memoriseTile %ix%i: %i\n", column, row, id);
    pair< map< unsigned int, Position_t>::iterator, bool> inserted = _seenTiles.insert( pair< unsigned int, Position_t>( id, Position_t( column, row)));
    bool learnedNewTile = inserted.second;
    if( !inserted.second && (inserted.first->second._column != column || inserted.first->second._row != row))
    {
        // different tile with same id was already in _seenTiles, move to _knownPairs:
        learnedNewTile = true;
        _knownPairs.insert(
            pair< unsigned int, pair< Position_t, Position_t> >(
                id, pair< Position_t, Position_t>(Position_t( column, row), inserted.first->second)));
        _seenTiles.erase( inserted.first);
        if (_verbose)
            printf("AI: learned a new pair! id: %i\n", id);
    }
    if ( learnedNewTile) {
        // the tile was unknown before, so delete it from _unknownTiles:
        for( uint i = 0; i < _unknownTiles.size(); ++i)
        {
            if ( (column == _unknownTiles[i]->_column && row == _unknownTiles[i]->_row) )
            {
                _unknownTiles[i] = _unknownTiles.back();
                _unknownTiles.pop_back();
                break;
            }
        }
    }
}

void MemoryAI::dememoriseTiles( unsigned int id, unsigned int column1, unsigned int row1, unsigned int column2, unsigned int row2)
{
    if (_verbose)
        printf("AI: dememoriseTile %i\n", id);
    int num_unknown_tiles = 2;
    map< unsigned int, pair< Position_t, Position_t> >::iterator itPair = _knownPairs.find( id);
    if( itPair != _knownPairs.end()) {
        _knownPairs.erase( itPair);
        num_unknown_tiles = 0;
    } else {
        map< unsigned int, Position_t>::iterator itSeenTile = _seenTiles.find( id);
        if( itSeenTile != _seenTiles.end()) {
            _seenTiles.erase( itSeenTile);
            num_unknown_tiles = 1;
        }
    }
    
    Position_t* pt;
    // if a tile was unknown before, remove it from _unknownTiles:
    if (num_unknown_tiles > 0)
        for( uint i = 0; i < _unknownTiles.size(); ++i)
        {
            pt = _unknownTiles[i];
            if ( (column1 == pt->_column && row1 == pt->_row) || (column2 == pt->_column && row2 == pt->_row))
            {
                _unknownTiles[i] = _unknownTiles.back();
                _unknownTiles.pop_back();
                --num_unknown_tiles;
                --i;
            }
            if (num_unknown_tiles == 0)
                break;
        }

    // delete tiles from _availableTiles:
    unsigned int deleteCounter = 2;
    for( unsigned int i = 0 ; i < _availableTiles.size() ; ++i)
    {
        pt = _availableTiles[i];
        //if( id == _unrevealedTiles[i]->_id) // geht nicht, _unrevealedTiles kennt die echte ID nicht.
        if( (column1 == pt->_column && row1 == pt->_row) || (column2 == pt->_column && row2 == pt->_row))
        {
            delete _availableTiles[i];
            _availableTiles[i] = NULL;
            // the following takes O(n-i) time, since all elements following the erased one must be shifted back:
            //_availableTiles.erase( _availableTiles.begin() + i);
            // since in our case, the vector must not be ordered at all, we can use the following trick:
            // (both swapping and pop_back takes constant time):
            _availableTiles[i] = _availableTiles.back();
            _availableTiles.pop_back();
            --deleteCounter;
            --i;
        }
        if( 0 == deleteCounter)
            break;
    }
}

Position_t* MemoryAI::randomGuess() const
{
    if (_useKnownPairs && _unknownTiles.size() > 0) {
        if (_verbose)
            printf("randomly choose a card not known yet\n");
        unsigned int randomIndex = rand() % _unknownTiles.size();
        return _unknownTiles[ randomIndex];
    } else {
        if (_verbose)
            printf("randomly choose any card\n");
        unsigned int randomIndex = rand() % _availableTiles.size();
        return _availableTiles[ randomIndex];
    }
}

void MemoryAI::revealedTile( unsigned int id, unsigned int column, unsigned int row)
{
    if (_verbose)
        printf("AI: revealedTile: %i x %i (id %i)\n", column, row, id);
    
	if( _isFirstTile)
	{
		_firstTile._id = id;
		_firstTile._column = column;
		_firstTile._row = row;
                _isFirstTile = false;

		// prüft, ob Tile schon als Paar bekannt ist
                map< unsigned int, pair< Position_t, Position_t> >::iterator it = _knownPairs.find( id);
		if( _knownPairs.end() != it)
			return;

		// fügt die Tile-Information dem Gedächtnis hinzu
		memoriseTile( id, column, row);
	}
	else
	{
            _isFirstTile = true;
		if( ( _firstTile._id == id) && (( _firstTile._column != column) || ( _firstTile._row != row)))
		{
			// paar gefunden -> lösche Paar aus dem Gedächtnis
			dememoriseTiles( id, _firstTile._column, _firstTile._row, column, row);
		}
		else
		{
			// kein paar gefunden
                    // prüft, ob Tile schon als Paar bekannt ist
                    if( _knownPairs.count( id) > 0) //TODO: da dies immer gemacht werden muss, bevor memoriseTile aufgerufen wird, koennen wir ueberlegen, ob wir es gleich in memoriseTile hinein verschieben
                        return;
			// fügt die Tile-Information dem Gedächtnis hinzu
			memoriseTile( id, column, row);
		}
	}
}

/*
void MemoryAI::guess()
{
	// schwierigkeitsgrad
	bool _useKnownPairs = true;
	if( (((double)qrand()) / (double)(RAND_MAX)) > _difficultyRate)
		_useKnownPairs = false;
	// anhand der vorliegenden Daten zwei Tiles wählen
	if( _knownPairs.empty() || !_useKnownPairs)
	{
		MTriple* firstGuess = randomGuess();
		unsigned int column = firstGuess->_column;
		unsigned int row = firstGuess->_row;
		// TODO: reveal first Tile
		blablubkeks( column, row);
		map< unsigned int, MTriple>::const_iterator itFind = _seenTiles.find( _firstTile._id);
		if( itFind == _seenTiles.end())
		{
			MTriple* secondGuess = randomGuess();
			while( firstGuess == secondGuess) // per Zufall kann das hier eine Weile "hängen bleiben"
				secondGuess = randomGuess();
			unsigned int column = secondGuess->_column;
			unsigned int row = secondGuess->_row;
			// TODO: reveal second Tile
			blablubkeks( column, row);
		}
		else
		{
			const MTriple& secondGuess = itFind->second;
			if( (((double)qrand()) / (double)(RAND_MAX)) > _difficultyRate)
				_useKnownPairs = false;
			else
				_useKnownPairs = true;
			if( ( firstGuess->_column != secondGuess._column) && ( firstGuess->_row != secondGuess._row) && _useKnownPairs)
			{
				unsigned int column = secondGuess._column;
				unsigned int row = secondGuess._row;
				// TODO: reveal second Tile
				blablubkeks( column, row);
			}
			else
			{
				MTriple* secondGuess = randomGuess();
				while( firstGuess == secondGuess) // per Zufall kann das hier eine Weile "hängen bleiben"
					secondGuess = randomGuess();
				unsigned int column = secondGuess->_column;
				unsigned int row = secondGuess->_row;
				// TODO: reveal second Tile
				blablubkeks( column, row);
			}
		}
	}
	else
	{
		const MTriple& firstGuess = _knownPairs.begin()->second.first;
		unsigned int column = firstGuess._column;
		unsigned int row = firstGuess._row;
		// TODO: reveal first Tile
		blablubkeks( column, row);
		const MTriple& secondGuess = _knownPairs.begin()->second.second;
		column = secondGuess._column;
		row = secondGuess._row;
		// TODO: reveal second Tile
		blablubkeks( column, row);
	}
}
*/

void MemoryAI::firstGuess()
{
    if (!_isFirstTile) {
        // just to be on the safe side
        printf("AI Warning: first guess demanded, but this is actually second guess\n");
        return secondGuess();
    }
    
    if (_verbose)
        printf("\nAI: first guess demanded\n");
	// schwierigkeitsgrad
	_useKnownPairs = true;
	if( (((double)qrand()) / (double)(RAND_MAX)) > _difficultyRate)
		_useKnownPairs = false;
        if (_verbose)
            printf("AI: will %suse known pairs\n", _useKnownPairs ? "" : "not ");
	// anhand der vorliegenden Daten zwei Tiles wählen
	if( _knownPairs.empty() || !_useKnownPairs)
	{
            if (_verbose)
                printf("AI: makes random guess\n");
		_firstGuess = randomGuess();
		unsigned int column = _firstGuess->_column;
		unsigned int row = _firstGuess->_row;
		// reveal first Tile:
                if (_verbose)
                    printf("AI: first guess (random): %i, %i\n", column, row);
                emit submitGuess(column, row);
		_status = RANDOM_GUESS;
		
	}
	else
	{
            if (_verbose)
                printf("AI: choose known pair\n");
		const Position_t& firstGuess = _knownPairs.begin()->second.first;
		unsigned int column = firstGuess._column;
		unsigned int row = firstGuess._row;
                /// reveal first Tile:
                if (_verbose)
                    printf("AI: first guess: %i, %i (known pair)\n", column, row);
                emit submitGuess(column, row);
		_status = KNOWS_PAIR;
	}
}


void MemoryAI::secondGuess()
{
    if (_isFirstTile) {
        // just to be on the safe side
        printf("AI Warning: second guess demanded, but this is actually first guess\n");
        return firstGuess();
    }
    
    if (_verbose)
        printf("AI: second guess demanded\n");
    map< unsigned int, pair< Position_t, Position_t> >::iterator itFind;
	int column = 0;
	int row = 0;
	Position_t* secondGuess = NULL;
	switch ( _status)
	{
	case KNOWS_PAIR:
		secondGuess = &_knownPairs.begin()->second.second;
		column = secondGuess->_column;
		row = secondGuess->_row;
                // reveal second Tile:
                if (_verbose)
                    printf("AI: second guess: %i, %i (known pair)\n", column, row);
                emit submitGuess(column, row);
		break;
	case RANDOM_GUESS:
		itFind = _knownPairs.find( _firstTile._id);
                if( itFind == _knownPairs.end())
		{
                    if (_verbose)
                        printf("AI: did not learn a new pair in the first move, make random 2nd guess\n");
			secondGuess = randomGuess();
			while( _firstGuess == secondGuess) // per Zufall kann das hier eine Weile "hängen bleiben"
				secondGuess = randomGuess();
			unsigned int column = secondGuess->_column;
			unsigned int row = secondGuess->_row;
                        // reveal second Tile:
                        if (_verbose)
                            printf("AI: second guess: %i, %i (random)\n", column, row);
                        emit submitGuess(column, row);
		}
		else
		{
                    if (_verbose)
                        printf("AI: knows a new pair because of first move\n");
			/* Das wurde in firstGuess schon gemacht:
                         * 
                         * if( (((double)qrand()) / (double)(RAND_MAX)) > _difficultyRate)
				_useKnownPairs = false;
			else
				_useKnownPairs = true;*/
			if( _useKnownPairs)
			{
                            if (_verbose)
                                printf("AI: knows a pair && use known pairs.\n");
                            secondGuess = &itFind->second.second;
                            // the previously known tile is always inserted second in memoriseTile, 
                            // so this should never happen, but just to be sure:
                            if (secondGuess->_column == _firstTile._column && secondGuess->_row == _firstTile._row)
                                secondGuess = &itFind->second.first;
                            
				unsigned int column = secondGuess->_column;
				unsigned int row = secondGuess->_row;
                                // reveal second Tile:
                                if (_verbose)
                                    printf("AI: second guess: %i, %i (known pair)\n", column, row);
                                emit submitGuess(column, row);
			}
			else
			{
                            if (_verbose)
                                printf("AI: knows pair but makes random guess\n");
				secondGuess = randomGuess();
				while( _firstGuess == secondGuess) // per Zufall kann das hier eine Weile "hängen bleiben"
					secondGuess = randomGuess();
				unsigned int column = secondGuess->_column;
				unsigned int row = secondGuess->_row;
                                // reveal second Tile
                                if (_verbose)
                                    printf("AI: second guess: %i, %i (random)\n", column, row);
                                emit submitGuess(column, row);
			}
		}
		break;
	default:
		// dieser Fall kann nicht eintreten
		break;
	}
}

void MemoryAI::setVerbosity(bool enabled)
{
    _verbose = enabled;
}


// necessary for Qt's meta objectc compiler, e.g. for signal-slot-system:
//#include "MemoryAI.moc"
