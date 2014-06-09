#!/bin/bash
mkdir simulationResult_rts
./rtsSimultation.sh
perl rtsperl.pl
gnuplot rts_cara_graph.gplt

mkdir simulationResult_frag
./fragSimulation.sh
perl fragperl.pl
gnuplot frag_cara_graph.gplt
