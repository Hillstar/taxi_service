#!/bin/bash

num_of_clients=$2;
num_of_taxi=$3;

# клиенты в очереди
for (( i = 0; i < (num_of_clients / 2); i++ )); 
    do
      gnome-terminal --geometry 60x35 -e "./client $1";
    done;

for (( i = 0; i < $num_of_taxi; i++ )); 
    do
      gnome-terminal --geometry 60x35 -e "./taxi $1";
    done;

# клиенты, выбирающие ближайшее такси
for (( i = 0; i < (num_of_clients / 2); i++ )); 
    do
      gnome-terminal --geometry 60x35 -e "./client $1";
    done;