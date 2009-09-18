 #*****************************************************************************\
 # FILE: slow_pow.r
 # DESCRIPTION: generates a graph of slowdown vs power savings and also 
 # plots the linear interpolation of these points with the formula for them
 # at the bottom left corner.
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

a = read.table('delta',header=TRUE)
x = a$slowdown[a$delta > 3]
y = (115 - a$avgpwr[a$delta > 3]) * 100 / 115

l = lm(y~x)

i = as.numeric(l$coefficients[1])
m = as.numeric(l$coefficients[2])

jpeg('test.jpg',width=1024,height=800)
plot(x,y,col='red',pch=3,xlab="Slowdown (%)", ylab="Power Savings (%)")
abline(i,m)
t = sprintf("Power savings = (%.3f x Slowdown) + %.3f",m,i);
text(1,0, t, col = "black", adj = c(0, -.1))
dev.off()

