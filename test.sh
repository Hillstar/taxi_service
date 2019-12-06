#!/bin/bash

num_of_clients=$2;
num_of_taxi=$3;
i = 0;

# клиенты в очереди
for (( i = 0; i < (num_of_clients / 2); i++ )); 
    do
      gnome-terminal --geometry 60x35 -e "./client $1 $i";
    done;

for (( j = 0; j < $num_of_taxi; j++ )); 
    do
      gnome-terminal --geometry 60x35 -e "./taxi $1 $j";
    done;

# клиенты, выбирающие ближайшее такси
for (( i; i < num_of_clients; i++ )); 
    do
      gnome-terminal --geometry 60x35 -e "./client $1 $i";
    done;