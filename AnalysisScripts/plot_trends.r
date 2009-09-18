 #*****************************************************************************\
 # FILE: plot_trends.r
 # DESCRIPTION: Plots the trends of epi, powersavings & slowdown along delta
 # and interval.
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

a = read.table('delta_sel', header=TRUE)

intervals = c(125,250,500,1000)
deltas = c(1,2,3,4,8,12,16)
cols = c('red', 'blue', 'green', 'black')
legname = c('125ms', '250ms', '500ms', '1000ms')
wrkpch = c(19, 19, 19, 19, 19, 19);

x = deltas;
y1 = c()
for(i in 1:length(deltas)){
	y1[i] = mean(a$slowdown[a$interval == intervals[1] & a$delta == deltas[i]]);
}
y2 = c()
for(i in 1:length(deltas)){
	y2[i] = mean(a$slowdown[a$interval == intervals[2] & a$delta == deltas[i]]);
}
y3 = c()
for(i in 1:length(deltas)){
	y3[i] = mean(a$slowdown[a$interval == intervals[3] & a$delta == deltas[i]]);
}
y4 = c()
for(i in 1:length(deltas)){
	y4[i] = mean(a$slowdown[a$interval == intervals[4] & a$delta == deltas[i]]);
}
m1 = max(max(y1),max(y2),max(y3),max(y4))
rg = c(0,m1)

jpeg('slowdown.jpg',width=1024,height=800)
plot(x,y1,col=cols[1],type='l',xlab="Delta", ylab="Slowdown (%)", axes=TRUE,ylim=rg)
lines(x,y2,col=cols[2], ylim=rg)
lines(x,y3,col=cols[3], ylim=rg)
lines(x,y4,col=cols[4], ylim=rg)
par(new=TRUE)
grid(col='gray');
par(new=TRUE)
legend('topright', legname, col=cols,pch=wrkpch);
dev.off();


x = deltas;
y1 = c()
for(i in 1:length(deltas)){
	y1[i] = mean(a$jpbi[a$interval == intervals[1] & a$delta == deltas[i]]);
}
y2 = c()
for(i in 1:length(deltas)){
	y2[i] = mean(a$jpbi[a$interval == intervals[2] & a$delta == deltas[i]]);
}
y3 = c()
for(i in 1:length(deltas)){
	y3[i] = mean(a$jpbi[a$interval == intervals[3] & a$delta == deltas[i]]);
}
y4 = c()
for(i in 1:length(deltas)){
	y4[i] = mean(a$jpbi[a$interval == intervals[4] & a$delta == deltas[i]]);
}
m0 = min(min(y1),min(y2),min(y3),min(y4))
m1 = max(max(y1),max(y2),max(y3),max(y4))
rg = c(m0,m1)

jpeg('jpbi.jpg',width=1024,height=800)
plot(x,y1,col=cols[1],type='l',xlab="Delta", ylab="Energy per Instruction (nJ/Instruction)", axes=TRUE,ylim=rg)
lines(x,y2,col=cols[2], ylim=rg)
lines(x,y3,col=cols[3], ylim=rg)
lines(x,y4,col=cols[4], ylim=rg)
par(new=TRUE)
grid(col='gray');
par(new=TRUE)
legend('topright', legname, col=cols,pch=wrkpch);
dev.off();


x = deltas;
y1 = c()
for(i in 1:length(deltas)){
	y1[i] = (115.0 - mean(a$avgpwr[a$interval == intervals[1] & a$delta == deltas[i]])) * 100 / 115.0;
}
y2 = c()
for(i in 1:length(deltas)){
	y2[i] = (115.0 - mean(a$avgpwr[a$interval == intervals[2] & a$delta == deltas[i]])) * 100 / 115.0;
}
y3 = c()
for(i in 1:length(deltas)){
	y3[i] = (115.0 - mean(a$avgpwr[a$interval == intervals[3] & a$delta == deltas[i]])) * 100 / 115.0;
}
y4 = c()
for(i in 1:length(deltas)){
	y4[i] = (115.0 - mean(a$avgpwr[a$interval == intervals[4] & a$delta == deltas[i]])) * 100 / 115.0;
}
m0 = min(min(y1),min(y2),min(y3),min(y4))
m1 = max(max(y1),max(y2),max(y3),max(y4))
rg = c(m0,m1)

jpeg('avgpwr.jpg',width=1024,height=800)
plot(x,y1,col=cols[1],type='l',xlab="Delta", ylab="Average Power savings (%)", axes=TRUE,ylim=rg)
lines(x,y2,col=cols[2], ylim=rg)
lines(x,y3,col=cols[3], ylim=rg)
lines(x,y4,col=cols[4], ylim=rg)
par(new=TRUE)
grid(col='gray');
par(new=TRUE)
legend('topright', legname, col=cols,pch=wrkpch);
dev.off();


