/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  Juergen <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
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
