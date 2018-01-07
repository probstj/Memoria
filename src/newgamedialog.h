/*
 * MIT License
 *
 * Copyright (c) 2014 Jürgen Probst
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

#ifndef NEWGAMEDIALOG_H
#define NEWGAMEDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QSettings>

enum OPPONENT
{
    NO_OPPONENT = 0,
    HUMAN_OPPONENT = 1,
    COMPUTER_OPPONENT = 2
};
enum STARTING_PLAYER
{
    HUMAN_STARTS = 0,
    COMPUTER_STARTS = 1,
    RANDOM_STARTS = 2
};

class NewGameDialog : public QDialog
{
    Q_OBJECT

public:  
    NewGameDialog(const int max_pairs, QWidget *parent = 0);
    
    void setMaxPairs(int maxPairs);

    int getNumberOfPairs() const { return _number_of_pairs; };
    OPPONENT getOpponent() const { return _opponent; };
    int getDifficultyLevel() const { return _difficulty_level; };
    STARTING_PLAYER getStartingPlayer() const { return _starting_player; };
    
private slots:
    void acceptValues();
    void rejectValues();
    
private:
    QSpinBox *_sb_number_of_pairs;
    QButtonGroup *_buttonGroup_opponent;
    QButtonGroup *_buttonGroup_difficulty;
    QButtonGroup *_buttonGroup_start;
    QDialogButtonBox *_dialogButtonBox;
    
    int _number_of_pairs;
    bool _computer_opponent;
    OPPONENT _opponent;
    STARTING_PLAYER _starting_player;
    int _difficulty_level;
    
};

#endif // NEWGAMEDIALOG_H
