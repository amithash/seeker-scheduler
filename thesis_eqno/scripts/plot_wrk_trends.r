
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
