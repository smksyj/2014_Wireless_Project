$filenameout="rtsthroughput.txt";
open(OUT, ">$filenameout") ||die "Failed opening.\n";
$nseed=5;

@threshold=(200,800,1400,2000);
foreach $stanum(1,2,3,5,10,20,50) {
	@throughput=(0,0,0,0);
	for ($thresIdx = 0; $thresIdx < 4; $thresIdx++) {
		@mean_per_seed=(0,0,0,0,0);
		for ($seed = 1; $seed <= $nseed; $seed++) {
			$filename="simulationResult_rts/nsta"."$stanum"."_rts"."$threshold[$thresIdx]"."_seed"."$seed".".txt";
			open(IN, "$filename") || die "Failed opning $filename.\n";
			$sum_per_seed=0;
			for($i = 0; $i < $stanum; $i++){
				$read_txt=<IN>;
				@temp=split(/\t/, $read_txt);
				chomp(@temp);
				$sum_per_seed += $temp[3]*8/1000/1000/9;
			}
			$mean_per_seed[$seed-1] = $sum_per_seed/$stanum;
		}
		$throughput[$thresIdx] = ($mean_per_seed[0] + $mean_per_seed[1] + $mean_per_seed[2] + $mean_per_seed[3] + $mean_per_seed[4]) / $nseed;
	}
	print OUT "$stanum $throughput[0] $throughput[1] $throughput[2] $throughput[3]\n";
}
