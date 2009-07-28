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

