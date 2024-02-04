# Ticks may not differ by more than 25%
sub are_cpu_balanced {
	my (@ticks) = @_;
	my (@sorted) = sort { $a <=> $b } @ticks;
	my ($imbalance) = $sorted[-1] - $sorted[0];
	if ($imbalance * 4 > $sorted[-1]) {
		fail "No load balancing detected: kernel ticks count in each CPU ",
		"may not differ by more than 25%\n";
	}
}

# Check that the number of kernel ticks on each processor is balanced
sub load_balance_check {
	my (@ticks) = @_;
	my $size = @ticks;
	if ($size < 2) {
		fail "Load balance test must run with multiple cpus, found ",
		"$size CPUs\n";
	}
	are_cpu_balanced(@ticks);
}

# Check that idle ticks does not exceed kernel ticks
# If it does, means that some CPU did not spend a majority of its time working
sub idle_check {
	my (@idle_ticks) = @{ $_[0] };
	my (@kernel_ticks) = @{ $_[1] };
	
	for my $i (0 .. $#idle_ticks) {
		if ($idle_ticks[$i] > $kernel_ticks[$i]) {
			fail "idle tick exceeds kernel ticks on CPU $i\n";
		}
	}		
}

1;
