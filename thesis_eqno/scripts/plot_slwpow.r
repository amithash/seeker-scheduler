
a = read.table('delta_sel', header=TRUE)

x = a$slowdown
y = (115.0 - a$avgpwr) * 100 / 115.0

jpeg('slowdown_vs_pwrsav.jpg',width=1024,height=800)
plot(x,y,pch=3,col='red',xlab="Slowdown (%)",ylab="Power Savings (%)")
dev.off()


