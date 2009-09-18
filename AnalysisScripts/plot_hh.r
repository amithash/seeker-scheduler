 #*****************************************************************************\
 # FILE: plot_hh.r
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

mp <- barplot2(t(as.matrix(hh)), beside = TRUE, col = c("lightgreen", "green", "blue", "red1", "red4"), ylab="Speedup from 1100MHz", legend = colnames(as.matrix(hh)), ylim = c(0, 2.9), main = "", sub = "Workload", col.sub = "gray10", cex.names = 1.5, plot.ci = TRUE, ci.l = t(as.matrix(l)), ci.u = t(as.matrix(h)), plot.grid = TRUE)
