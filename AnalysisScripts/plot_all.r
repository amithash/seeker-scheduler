 #*****************************************************************************\
 # FILE: plot_all.r
 # DESCRIPTION: This massively inefficient and large R script takes in the 
 # high level table values of delta, ondemand and delta_sel and generates 3
 # graphs for each value of delta of scatter plots in each way.
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

a = read.table('delta',header=TRUE,as.is=FALSE)
b = read.table('ondemand', header=TRUE, as.is=FALSE)
c = read.table('delta_sel', header=TRUE, as.is=FALSE)

a$avgpwr = (115.0 - a$avgpwr) * 100 / 115.0
b$avgpwr = (115.0 - b$avgpwr) * 100 / 115.0
c$avgpwr = (115.0 - c$avgpwr) * 100 / 115.0

a.1 = subset(a,a["workload"] == "High")
a.2 = subset(a,a["workload"] == "Low")
a.3 = subset(a,a["workload"] == "Low-High")
a.4 = subset(a,a["workload"] == "PHigh-PLow")
a.5 = subset(a,a["workload"] == "PLow-High")
a.6 = subset(a,a["workload"] == "PLow-Low")
b.1 = subset(b,b["workload"] == "High")
b.2 = subset(b,b["workload"] == "Low")
b.3 = subset(b,b["workload"] == "Low-High")
b.4 = subset(b,b["workload"] == "PHigh-PLow")
b.5 = subset(b,b["workload"] == "PLow-High")
b.6 = subset(b,b["workload"] == "PLow-Low")
c.1 = subset(c,c["workload"] == "High")
c.2 = subset(c,c["workload"] == "Low")
c.3 = subset(c,c["workload"] == "Low-High")
c.4 = subset(c,c["workload"] == "PHigh-PLow")
c.5 = subset(c,c["workload"] == "PLow-High")
c.6 = subset(c,c["workload"] == "PLow-Low")

# Paremeters
plot_cex = 1.2
delta_pch = 3
ondemand_pch = 19
delta_sel_pch = 2

high_col='red'
low_col = 'green'
low_high_col = 'blue'
phigh_plow_col = 'skyblue'
plow_high_col = 'orange'
plow_low_col = 'brown'

grid_col = 'gray'

pwrlow = min(min(a['avgpwr']),min(b['avgpwr']));
pwrlow = (floor(pwrlow / 10) - 1) * 10;
pwrhigh = max(max(a['avgpwr']), max(b['avgpwr']));
pwrhigh = (floor(pwrhigh / 10) + 1) * 10;

jpbilow = min(min(a['jpbi']), min(b['jpbi']));
jpbilow = (floor(jpbilow / 10) - 1) * 10;
jpbihigh = max(max(a['jpbi']), max(b['jpbi']));
jpbihigh = (floor(jpbihigh / 10) + 1) * 10;

slowlow = min(min(a['slowdown']), min(b['slowdown']));
slowlow = (floor(slowlow / 10) - 1) * 10;
slowhigh = max(max(a['slowdown']), max(b['slowdown']));
slowhigh = (floor(slowhigh / 10) + 1) * 10;

pwr_vs_jpbi = function(outg, reqdelta){
	xlow = pwrlow;
	xhigh = pwrhigh;
	ylow = jpbilow;
	yhigh = jpbihigh;
	jpeg(outg,width=1024,height=800)
	plot(data=subset(a.1,a.1['delta'] == reqdelta),jpbi~avgpwr,col=high_col,pch=delta_pch,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh), xlab="Average Power Savings (%)", ylab="Energy per Instruction (Joules / Billion Instructions)",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.2,a.2['delta'] == reqdelta),jpbi~avgpwr,col=low_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh), xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.3,a.3['delta'] == reqdelta),jpbi~avgpwr,col=low_high_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.4,a.4['delta'] == reqdelta),jpbi~avgpwr,col=phigh_plow_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.5,a.5['delta'] == reqdelta),jpbi~avgpwr,col=plow_high_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="",ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.6,a.6['delta'] == reqdelta),jpbi~avgpwr,col=plow_low_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	
	plot(data=b.1,jpbi~avgpwr,col=high_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.2,jpbi~avgpwr,col=low_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.3,jpbi~avgpwr,col=low_high_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.4,jpbi~avgpwr,col=phigh_plow_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.5,jpbi~avgpwr,col=plow_high_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.6,jpbi~avgpwr,col=plow_low_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);

	plot(data=subset(c.1,c.1['delta'] == reqdelta),jpbi~avgpwr,col=high_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh), xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.2,c.2['delta'] == reqdelta),jpbi~avgpwr,col=low_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh), xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.3,c.3['delta'] == reqdelta),jpbi~avgpwr,col=low_high_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.4,c.4['delta'] == reqdelta),jpbi~avgpwr,col=phigh_plow_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.5,c.5['delta'] == reqdelta),jpbi~avgpwr,col=plow_high_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="",ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.6,c.6['delta'] == reqdelta),jpbi~avgpwr,col=plow_low_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);

	grid(col=grid_col);
	par(new=TRUE);
	workload = c('High', 'Low', 'Low-High', 'PHigh-PLow', 'PLow-PHigh', 'PLow-Low');
	wrkcol = c(high_col, low_col, low_high_col, phigh_plow_col, plow_high_col, plow_low_col);
	wrkpch = c(19, 19, 19, 19, 19, 19);
	legend('topleft', workload, col=wrkcol,pch=wrkpch, text.col=wrkcol);
	par(new=TRUE);
	legend('bottomleft', c('Delta + Ladder', 'Delta + Select', 'Ondemand'), col=c('black','black', 'black'), pch=c(delta_pch, delta_sel_pch, ondemand_pch))
	dev.off();
}
pwr_vs_slowdown = function(outg, reqdelta){
	xlow = pwrlow;
	xhigh = pwrhigh;
	ylow = slowlow;
	yhigh = slowhigh;
	jpeg(outg,width=1024,height=800)
	plot(data=subset(a.1,a.1['delta'] == reqdelta),slowdown~avgpwr,col=high_col,pch=delta_pch,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh), xlab="Average Power Savings (%)", ylab="Slowdown (%)",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.2,a.2['delta'] == reqdelta),slowdown~avgpwr,col=low_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh), xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.3,a.3['delta'] == reqdelta),slowdown~avgpwr,col=low_high_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.4,a.4['delta'] == reqdelta),slowdown~avgpwr,col=phigh_plow_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.5,a.5['delta'] == reqdelta),slowdown~avgpwr,col=plow_high_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="",ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.6,a.6['delta'] == reqdelta),slowdown~avgpwr,col=plow_low_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	
	plot(data=b.1,slowdown~avgpwr,col=high_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.2,slowdown~avgpwr,col=low_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.3,slowdown~avgpwr,col=low_high_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.4,slowdown~avgpwr,col=phigh_plow_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.5,slowdown~avgpwr,col=plow_high_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.6,slowdown~avgpwr,col=plow_low_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);

	plot(data=subset(c.1,c.1['delta'] == reqdelta),slowdown~avgpwr,col=high_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh), xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.2,c.2['delta'] == reqdelta),slowdown~avgpwr,col=low_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh), xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.3,c.3['delta'] == reqdelta),slowdown~avgpwr,col=low_high_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.4,c.4['delta'] == reqdelta),slowdown~avgpwr,col=phigh_plow_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.5,c.5['delta'] == reqdelta),slowdown~avgpwr,col=plow_high_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="",ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.6,c.6['delta'] == reqdelta),slowdown~avgpwr,col=plow_low_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	grid(col=grid_col);
	par(new=TRUE);
	workload = c('High', 'Low', 'Low-High', 'PHigh-PLow', 'PLow-PHigh', 'PLow-Low');
	wrkcol = c(high_col, low_col, low_high_col, phigh_plow_col, plow_high_col, plow_low_col);
	wrkpch = c(19, 19, 19, 19, 19, 19);
	legend('topleft', workload, col=wrkcol,pch=wrkpch, text.col=wrkcol);
	par(new=TRUE);
	legend('bottomleft', c('Delta + Ladder', 'Delta + Select', 'Ondemand'), col=c('black','black', 'black'), pch=c(delta_pch, delta_sel_pch, ondemand_pch))
	dev.off();
}

jpbi_vs_slowdown = function(outg, reqdelta){
	xlow = jpbilow;
	xhigh = jpbihigh;
	ylow = slowlow;
	yhigh = slowhigh;
	jpeg(outg,width=1024,height=800)
	plot(data=subset(a.1,a.1['delta'] == reqdelta),slowdown~jpbi,col=high_col,pch=delta_pch,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh), ylab="Slowdown (%)", xlab="Energy per Instruction (Joules / Billion Instructions)",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.2,a.2['delta'] == reqdelta),slowdown~jpbi,col=low_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh), xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.3,a.3['delta'] == reqdelta),slowdown~jpbi,col=low_high_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.4,a.4['delta'] == reqdelta),slowdown~jpbi,col=phigh_plow_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.5,a.5['delta'] == reqdelta),slowdown~jpbi,col=plow_high_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="",ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(a.6,a.6['delta'] == reqdelta),slowdown~jpbi,col=plow_low_col,pch=delta_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	
	plot(data=b.1,slowdown~jpbi,col=high_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.2,slowdown~jpbi,col=low_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.3,slowdown~jpbi,col=low_high_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.4,slowdown~jpbi,col=phigh_plow_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.5,slowdown~jpbi,col=plow_high_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=b.6,slowdown~jpbi,col=plow_low_col,pch=ondemand_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);

	plot(data=subset(c.1,c.1['delta'] == reqdelta),slowdown~jpbi,col=high_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh), ylab="", xlab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.2,c.2['delta'] == reqdelta),slowdown~jpbi,col=low_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh), xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.3,c.3['delta'] == reqdelta),slowdown~jpbi,col=low_high_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.4,c.4['delta'] == reqdelta),slowdown~jpbi,col=phigh_plow_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.5,c.5['delta'] == reqdelta),slowdown~jpbi,col=plow_high_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="",ylab="",cex=plot_cex);
	par(new=TRUE);
	plot(data=subset(c.6,c.6['delta'] == reqdelta),slowdown~jpbi,col=plow_low_col,pch=delta_sel_pch,axes=FALSE,xlim=range(xlow,xhigh), ylim=range(ylow,yhigh),xlab="", ylab="",cex=plot_cex);
	par(new=TRUE);
	grid(col=grid_col);
	par(new=TRUE);
	workload = c('High', 'Low', 'Low-High', 'PHigh-PLow', 'PLow-PHigh', 'PLow-Low');
	wrkcol = c(high_col, low_col, low_high_col, phigh_plow_col, plow_high_col, plow_low_col);
	wrkpch = c(19, 19, 19, 19, 19, 19);
	legend('topleft', workload, col=wrkcol,pch=wrkpch, text.col=wrkcol);
	par(new=TRUE);
	legend('bottomleft', c('Delta + Ladder', 'Delta + Select', 'Ondemand'), col=c('black','black', 'black'), pch=c(delta_pch, delta_sel_pch, ondemand_pch))
	dev.off();
}

deltas = c(1,2,3,4,8,12,16)
for(i in 1:length(deltas)){
	filename_pwr_vs_jpbi = paste("pwr_vs_jpbi_delta_", toString(deltas[i]), ".jpg", sep="")
	filename_pwr_vs_slowdown = paste("pwr_vs_slowdown_delta_", toString(deltas[i]), ".jpg", sep="")
	filename_jpbi_vs_slowdown = paste("jpbi_vs_slowdown_delta_", toString(deltas[i]), ".jpg", sep="")
	pwr_vs_jpbi(filename_pwr_vs_jpbi, deltas[i])
	pwr_vs_slowdown(filename_pwr_vs_slowdown, deltas[i])
	jpbi_vs_slowdown(filename_jpbi_vs_slowdown, deltas[i])

}
