a = read.table('delta', header=TRUE, as.is=FALSE)
a$interval = as.factor(a$interval)
a$delta = as.factor(a$delta)
a$jpbi = as.numeric(a$jpbi)
a$avgpwr = as.numeric(a$avgpwr)
a$slowdown = as.numeric(a$slowdown)
ll = 1.5
plot_bench = function(bench) {
	name_epi = paste('trends_epi_',bench,".jpg",sep='')
	name_pwr = paste('trends_pwr_',bench,".jpg",sep='')
	name_slw = paste('trends_slw_',bench,".jpg",sep='')
	main_epi = paste("EPI Trend for Workload",bench)
	main_pwr = paste("Average Power Consumption Trend for Workload",bench)
	main_slw = paste("Slowdown Trend for Workload",bench)
	jpeg(name_epi,width=1024,height=800)
	interaction.plot(a$delta[a$workload == bench],a$interval[a$workload == bench],a$jpbi[a$workload == bench],xlab="delta",ylab="Energy per Instruction (nJ/I)",col=c('Red','brown','blue','green'),lty=c(1,1,1,1),trace.label="Interval",fun=mean,fixed=TRUE, lwd=c(ll,ll,ll,ll),main=main_epi)
	par(new=TRUE);
	grid(col='black');
	dev.off();

	jpeg(name_pwr,width=1024,height=800)
	interaction.plot(a$delta[a$workload == bench],a$interval[a$workload == bench],a$avgpwr[a$workload == bench],xlab="delta",ylab="Average Power (Watts)",col=c('Red','brown','blue','green'),lty=c(1,1,1,1),trace.label="Interval",fun=mean,fixed=TRUE, lwd=c(ll,ll,ll,ll), main=main_pwr)
	par(new=TRUE);
	grid(col='black');
	dev.off();

	jpeg(name_slw,width=1024,height=800)
	interaction.plot(a$delta[a$workload == bench],a$interval[a$workload == bench],a$slowdown[a$workload == bench],xlab="delta",ylab="Slowdown (%)",col=c('Red','brown','blue','green'),lty=c(1,1,1,1),trace.label="Interval", fun=mean, fixed=TRUE, lwd = c(ll,ll,ll,ll), main=main_slw)
	par(new=TRUE);
	grid(col='black');
	dev.off();
}
benches = c('High','Low', 'Low-High', 'PHigh-PLow', 'PLow-High', 'PLow-Low')

for(i in 1:length(benches)){
	plot_bench(benches[i])
}
	

