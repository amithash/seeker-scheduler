 #*****************************************************************************\
 # FILE: plot_slwpwr.r
 # DESCRIPTION: 
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

x = a$slowdown
y = (115.0 - a$avgpwr) * 100 / 115.0

jpeg('slowdown_vs_pwrsav.jpg',width=1024,height=800)
plot(x,y,pch=3,col='red',xlab="Slowdown (%)",ylab="Power Savings (%)")
dev.off()


