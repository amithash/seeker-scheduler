 #*****************************************************************************\
 # FILE: plot_wrk_trends.r
 # DESCRIPTION: Prints a box plot of Powersavings and slowdown for each 
 # workload.
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

a = read.table('delta_sel',header=TRUE);

frac = 100/6
x = c(0,frac,2*frac, 3*frac,4*frac,5*frac,6*frac)

jpeg('wrk_avgpwr.jpg',width=1024,height=800)
plot(a$workload, (115 - a$avgpwr) * 100 / 115,col='green',ylim=c(0,60),xlab="Workload",ylab="Percentage Change (%)",y2lab="Power Savings (%)")
par(new=TRUE)
plot(a$workload, a$slowdown,col='red',ylim=c(0,100),xlab="",ylab="",axes=FALSE)
axis(4,at=x,c(0,10,20,30,40,50,60),main="kakka")
par(new=TRUE)
legend('topright',c('Slowdown', 'Power Savings'), col=c('red', 'green'), pch=c(19,19))
par(new=TRUE)
grid(col='grey')

dev.off()
#jpeg('wrk_slowdown.jpg',width=1024,height=800)
#plot(a$workload, a$slowdown,col='red',ylim=c(0,100))
#dev.off()
