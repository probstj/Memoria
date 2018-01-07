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

#include "newgamedialog.h"

NewGameDialog::NewGameDialog(const int max_pairs, QWidget* parent): QDialog(parent)
{
    // number of pairs:
    QLabel *label = new QLabel(tr("&Number of card pairs:"), this);
    _sb_number_of_pairs = new QSpinBox(this);
    _sb_number_of_pairs->setMinimum(2);
    _sb_number_of_pairs->setMaximum(max_pairs);
    label->setBuddy(_sb_number_of_pairs);
    
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(label);
    topLayout->addWidget(_sb_number_of_pairs);
    
    // players: 
    QRadioButton *rb_single_player = new QRadioButton(tr("&Single player game"), this);
    QRadioButton *rb_human_opponent = new QRadioButton(tr("&Two player game"), this);
    QRadioButton *rb_computer_opponent = new QRadioButton(tr("Play against &computer:"), this);
    _buttonGroup_opponent = new QButtonGroup(this);
    _buttonGroup_opponent->addButton(rb_single_player, NO_OPPONENT);
    _buttonGroup_opponent->addButton(rb_human_opponent, HUMAN_OPPONENT);
    _buttonGroup_opponent->addButton(rb_computer_opponent, COMPUTER_OPPONENT);
    
    // computer opponent difficulty level:
    QGroupBox *groupBox1 = new QGroupBox(tr("Difficulty"), this);
    QRadioButton *rb_ai_level1 = new QRadioButton(tr("&1: easy game"), this);
    QRadioButton *rb_ai_level2 = new QRadioButton(tr("&2: normal game"), this);
    QRadioButton *rb_ai_level3 = new QRadioButton(tr("&3: hard game"), this);
    QRadioButton *rb_ai_level4 = new QRadioButton(tr("&4: impossible game"), this);
    _buttonGroup_difficulty = new QButtonGroup(this);
    _buttonGroup_difficulty->addButton(rb_ai_level1, 1);
    _buttonGroup_difficulty->addButton(rb_ai_level2, 2);
    _buttonGroup_difficulty->addButton(rb_ai_level3, 3);
    _buttonGroup_difficulty->addButton(rb_ai_level4, 4);
 
    QVBoxLayout *gbLayout = new QVBoxLayout;
    gbLayout->addWidget(rb_ai_level1);
    gbLayout->addWidget(rb_ai_level2);
    gbLayout->addWidget(rb_ai_level3);
    gbLayout->addWidget(rb_ai_level4);
    groupBox1->setLayout(gbLayout);
    
    // computer opponent starting player:
    QGroupBox *groupBox2 = new QGroupBox(tr("Starting player"), this);
    QRadioButton *rb_human_starts = new QRadioButton(tr("&human player starts"), this);
    QRadioButton *rb_ai_starts = new QRadioButton(tr("com&puter starts"), this);
    QRadioButton *rb_random_starts = new QRadioButton(tr("&random player starts"), this);
    _buttonGroup_start = new QButtonGroup(this);
    _buttonGroup_start->addButton(rb_human_starts, HUMAN_STARTS);
    _buttonGroup_start->addButton(rb_ai_starts, COMPUTER_STARTS);
    _buttonGroup_start->addButton(rb_random_starts, RANDOM_STARTS);
    
    gbLayout = new QVBoxLayout;
    gbLayout->addWidget(rb_human_starts);
    gbLayout->addWidget(rb_ai_starts);
    gbLayout->addWidget(rb_random_starts);
    groupBox2->setLayout(gbLayout);
    
    // OK & Cancel buttons:
    _dialogButtonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, 
        Qt::Horizontal, this);
    
    // add all to main layout:
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(rb_single_player);
    mainLayout->addWidget(rb_human_opponent);
    mainLayout->addWidget(rb_computer_opponent);
    mainLayout->addWidget(groupBox1);
    mainLayout->addWidget(groupBox2);
    mainLayout->addSpacing(20);
    mainLayout->addStretch();
    mainLayout->addWidget(_dialogButtonBox);
    
    setContentsMargins(20, 20, 20, 0);
    setLayout(mainLayout);
    setWindowTitle(tr("Start new game"));
    
    connect(_dialogButtonBox, SIGNAL(accepted()), this, SLOT(acceptValues()));
    connect(_dialogButtonBox, SIGNAL(rejected()), this, SLOT(rejectValues()));
    connect(rb_computer_opponent, SIGNAL(toggled(bool)), groupBox1, SLOT(setEnabled(bool)));
    connect(rb_computer_opponent, SIGNAL(toggled(bool)), groupBox2, SLOT(setEnabled(bool)));
    
    // read init values from INI file:
    QSettings settings;
    settings.beginGroup("GameSettings");
    int numpairs = settings.value("number_of_pairs", 16).toInt();
    int opponent = settings.value("opponent", COMPUTER_OPPONENT).toInt(); // single: 0, computer: 1, other human: 2
    int difficulty = settings.value("difficulty_level", 2).toInt();
    int starting_player = settings.value("starting_player", HUMAN_STARTS).toInt();
    settings.endGroup();
    
    _number_of_pairs = max_pairs >= numpairs ? numpairs : max_pairs;
    _computer_opponent = opponent == COMPUTER_OPPONENT;
    _opponent = (OPPONENT)opponent;
    _difficulty_level = difficulty;
    _starting_player = (STARTING_PLAYER)starting_player;
    _sb_number_of_pairs->setValue(_number_of_pairs);
    _buttonGroup_opponent->button(_opponent)->setChecked(true);
    _buttonGroup_difficulty->button(_difficulty_level)->setChecked(true);
    _buttonGroup_start->button(_starting_player)->setChecked(true);
    groupBox1->setEnabled(opponent == COMPUTER_OPPONENT);
    groupBox2->setEnabled(opponent == COMPUTER_OPPONENT);
}

void NewGameDialog::setMaxPairs(int maxPairs) {
    _sb_number_of_pairs->setMaximum(maxPairs);
}

void NewGameDialog::acceptValues()
{
    _number_of_pairs = _sb_number_of_pairs->value();
    _opponent = (OPPONENT)_buttonGroup_opponent->checkedId();
    _difficulty_level = _buttonGroup_difficulty->checkedId();
    _starting_player = (STARTING_PLAYER)_buttonGroup_start->checkedId();
    
    // save values to INI file so the game starts with the same initial settings the 
    // next time the program starts:
    QSettings settings;
    settings.beginGroup("GameSettings");
    settings.setValue("number_of_pairs", _number_of_pairs);
    settings.setValue("opponent", (int)_opponent);
    settings.setValue("difficulty_level", _difficulty_level);
    settings.setValue("starting_player", (int)_starting_player);
    settings.endGroup();
    settings.sync();
    
    accept();
}

void NewGameDialog::rejectValues()
{
    _sb_number_of_pairs->setValue(_number_of_pairs);
    _buttonGroup_opponent->button(_opponent)->setChecked(true);
    _buttonGroup_difficulty->button(_difficulty_level)->setChecked(true);
    _buttonGroup_start->button(_starting_player)->setChecked(true);
    reject();
}


//#include "newgamedialog.moc"
