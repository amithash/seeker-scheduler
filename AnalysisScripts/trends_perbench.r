 #*****************************************************************************\
 # FILE: trends_perbench.r
 # DESCRIPTION: Generates an interaction plot of EPI, Avg power consumption and
 # slowdown with respect to delta and interval for each workload.
 #
 #*****************************************************************************/

 #*****************************************************************************\
 # Copyright 2009 Amithash Prasad                                              *
 #                                                                             *
 # This file is part of Seeker                                                 *
 #                                                                             *
 # Seeker is free software: you can redistribute it and/or modify it under the *
 # terms of the GNU General Public License as published by the Free Software   *
 # Foundation, either version 3 of the License, or (at your option) any later  *
 # version.                                                                    *
 #                                                                             *
 # This program is distributed in the hope that it will be useful, but WITHOUT *
 # ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 # FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License        *
 # for more details.                                                           *
 #                                                                             *
 # You should have received a copy of the GNU General Public License along     *
 # with this program. If not, see <http://www.gnu.org/licenses/>.              *
 #*****************************************************************************/
  
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
	

