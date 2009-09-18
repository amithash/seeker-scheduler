 #*****************************************************************************\
 # FILE: analyze.r
 # DESCRIPTION: For 3 trials of experiments for delta, deltasel, ondemand and
 # fixed, this takes in files (Hardcoded) which are the output of epi.pl which
 # itself takes in the output of analyzeDVFS.pl. And finally generates 
 # a file with a computed value for slowdown as a additional column, with the
 # rest being medians.
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

# Load the data files.
delta5_t1 = read.table('jpbi_delta5_t1',header=TRUE,as.is=TRUE)
delta5_t2 = read.table('jpbi_delta5_t2',header=TRUE,as.is=TRUE)
delta5_t3 = read.table('jpbi_delta5_t3',header=TRUE,as.is=TRUE)
delta_sel_t1 = read.table('jpbi_delta_sel_t1',header=TRUE,as.is=TRUE)
delta_sel_t2 = read.table('jpbi_delta_sel_t2',header=TRUE,as.is=TRUE)
delta_sel_t3 = read.table('jpbi_delta_sel_t3',header=TRUE,as.is=TRUE)
ondemand_t1 = read.table('jpbi_ondemand_t1',header=TRUE,as.is=TRUE)
ondemand_t2 = read.table('jpbi_ondemand_t2',header=TRUE,as.is=TRUE)
ondemand_t3 = read.table('jpbi_ondemand_t3',header=TRUE,as.is=TRUE)
fixed_t1 = read.table('jpbi_fixed_t1',header=TRUE,as.is=TRUE)
fixed_t2 = read.table('jpbi_fixed_t2',header=TRUE,as.is=TRUE)
fixed_t3 = read.table('jpbi_fixed_t3',header=TRUE,as.is=TRUE)

# Get all of them. 
delta5.all = rbind(delta5_t1, delta5_t2, delta5_t3);
delta_sel.all = rbind(delta_sel_t1, delta_sel_t2, delta_sel_t3);
fixed.all = rbind(fixed_t1, fixed_t2, fixed_t3);
ondemand.all = rbind(ondemand_t1, ondemand_t2, ondemand_t3);

# Getting the fastest times... 
fastest = subset(fixed.all, layout == '[0,0,0,0,4]');
# row.names(fastest) = 1:length(row.names(fastest))
fast = c(as.numeric(median(subset(fastest,workload == 'High')$time)),as.numeric(median(subset(fastest,workload == 'High')$jpbi)),as.numeric(median(subset(fastest,workload == 'High')$avgpwr)))
fast = rbind(fast, c(as.numeric(median(subset(fastest,workload == 'Low')$time)), as.numeric(median(subset(fastest,workload == 'Low')$jpbi)), as.numeric(median(subset(fastest,workload == 'Low')$avgpwr))))
fast = rbind(fast, c(as.numeric(median(subset(fastest,workload == 'Low-High')$time)), as.numeric(median(subset(fastest,workload == 'Low-High')$jpbi)), as.numeric(median(subset(fastest,workload == 'Low-High')$avgpwr))))
fast = rbind(fast, c(as.numeric(median(subset(fastest,workload == 'PHigh-PLow')$time)), as.numeric(median(subset(fastest,workload == 'PHigh-PLow')$jpbi)), as.numeric(median(subset(fastest,workload == 'PHigh-PLow')$avgpwr))))
fast = rbind(fast, c(as.numeric(median(subset(fastest,workload == 'PLow-High')$time)), as.numeric(median(subset(fastest,workload == 'PLow-High')$jpbi)), as.numeric(median(subset(fastest,workload == 'PLow-High')$avgpwr))))
fast = rbind(fast, c(as.numeric(median(subset(fastest,workload == 'PLow-Low')$time)), as.numeric(median(subset(fastest,workload == 'PLow-Low')$jpbi)), as.numeric(median(subset(fastest,workload == 'PLow-Low')$avgpwr))))
fast = data.frame(fast, row.names=c('High', 'Low', 'Low-High', 'PHigh-PLow', 'PLow-High', 'PLow-Low'))
names(fast) = c('time', 'jpbi', 'avgpwr')

# From now if you want the base fastest time for workload a = 'High', then just use fast[a,1]

slowdown = function (this_time, fast_time) ((this_time - fast_time) * 100 / fast_time)

vect_slowdown = function(this_time){
	ret = cbind(this_time,this_time$time);
	names(ret) = c(names(this_time),'slowdown');
	for(i in 1:length(this_time$time)){
		ret$slowdown[i] = slowdown(this_time$time[i], fast[this_time$workload[i], 'time']);
	}
	ret
}

vect_median = function(list1, list2, list3) 
{
	ret = list1;
	for(i in 1:length(list1$time)){
		ret$time[i] = median(c(list1$time[i], list2$time[i], list3$time[i]))
		ret$avgpwr[i] = median(c(list1$avgpwr[i], list2$avgpwr[i], list3$avgpwr[i]))
		ret$jpbi[i] = median(c(list1$jpbi[i], list2$jpbi[i], list3$jpbi[i]))
	}
	ret
}
delta = vect_slowdown(rbind(delta5_t1, delta5_t2, delta5_t3))
delta_sel = vect_slowdown(rbind(delta_sel_t1, delta_sel_t2, delta_sel_t3))
ondemand = vect_slowdown(rbind(ondemand_t1, ondemand_t2, ondemand_t3))
fixed = vect_slowdown(rbind(fixed_t1, fixed_t2, fixed_t3))
write.table(delta, file="delta", quote=FALSE, row.names=FALSE)
write.table(delta_sel, file="delta_sel", quote=FALSE, row.names=FALSE)
write.table(ondemand, file="ondemand", quote=FALSE, row.names=FALSE)
write.table(fixed, file="fixed", quote=FALSE, row.names=FALSE)


