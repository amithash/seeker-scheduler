a = read.table('delta_sel', header=TRUE, as.is=FALSE)
a$interval = as.factor(a$interval)
a$delta = as.factor(a$delta)
a$jpbi = as.numeric(a$jpbi)
a$avgpwr = as.numeric(a$avgpwr)
a$slowdown = as.numeric(a$slowdown)
ll = 1.5

jpeg('trends_epi.jpg',width=1024,height=800)
interaction.plot(a$delta,a$interval,a$jpbi,xlab="delta",ylab="Energy per Instruction (nJ/I)",col=c('Red','brown','blue','green'),lty=c(1,1,1,1),trace.label="Interval",fun=mean,fixed=TRUE, lwd=c(ll,ll,ll,ll))
par(new=TRUE);
grid(col='black');
dev.off();

jpeg('trends_pwr.jpg',width=1024,height=800)
interaction.plot(a$delta,a$interval,a$avgpwr,xlab="delta",ylab="Average Power (Watts)",col=c('Red','brown','blue','green'),lty=c(1,1,1,1),trace.label="Interval",fun=mean,fixed=TRUE, lwd=c(ll,ll,ll,ll))
par(new=TRUE);
grid(col='black');
dev.off();

jpeg('trends_slowdown.jpg',width=1024,height=800)
interaction.plot(a$delta,a$interval,a$slowdown,xlab="delta",ylab="Slowdown (%)",col=c('Red','brown','blue','green'),lty=c(1,1,1,1),trace.label="Interval", fun=mean, fixed=TRUE, lwd = c(ll,ll,ll,ll))
par(new=TRUE);
grid(col='black');
dev.off();


	
