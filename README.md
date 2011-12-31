# Nomis Memory Game V01

A Simon-like Memory Game for the ATTiny85 (+ some electronics)!

2011 (c) Christopher Woodall
MIT License

## Overview

Nomis is a Simon-like Memory game for the ATTiny85 (+ some electronics). The 
goal of the game is to copy the random string of moves that the 
microcontroller, makes. If you make a wrong move you go back to the IDLE state
(the lights cascade). After you copy the string so far, the computer will
display the string again and then add an extra move. The process repeats,
until you mess up. The goal of the game is not to win (since there are no
real winning conditions), the goal is to beat your previous score and 
"improve your memory".

## Layout

nomis-memory-game.c: This is the main game file, which controls the game logic.

scripts/lcg.py: A little test of the Linear Congruential Generator, which I 
used to generate random numbers

scripts/lfsr.py: A little test of a Linear Feedback Shift Register, which was
another option for my random number generator

schematics/nomis-memory-game-v01.sch: EAGLE schematic for the game

##
