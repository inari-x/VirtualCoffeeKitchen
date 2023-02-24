#!/bin/bash
for (( i=0; i<6; i++))
do
    sleep 9
    PARTICIPANT_COUNT=$(curl http://localhost:80/assign-participant-count-variable)
done