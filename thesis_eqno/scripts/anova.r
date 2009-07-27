require(lattice)
a = read.table('jpbi_delta5',header=TRUE,as.is=TRUE)
a$interval = as.factor(a$interval);
a$delta = as.factor(a$delta);
workloads = c('High', 'Low', 'Low-High', 'PHigh-PLow', 'PLow-High', 'PLow-Low');

print.aov.wrk = function(work){
	tmp = subset(a, workload == work)
	tmp1 = aov(data=tmp,jpbi~delta*interval);
	tmp2 = aov(data=tmp,avgpwr~delta*interval);
	tmp3 = aov(data=tmp,slowdown~delta*interval);
#	print(work);
#	print('JBPI');
#	print(summary(tmp1))
#	print(model.tables(tmp1,"means"),digits=3)
#	print("---------------------------------------------------------------")
#	print('AVGPWR');
#	summary(tmp2)
#	print(model.tables(tmp2,"means"),digits=3)
#	print("---------------------------------------------------------------")
#	print('SLOWDOWN');
#	summary(tmp3)
#	print(model.tables(tmp3,"means"),digits=3)
#	print("---------------------------------------------------------------")
##	print("---------------------------------------------------------------")	
	n = paste(work,".pdf",sep="");
	pdf(n)
	print(levelplot(as.table(model.tables(tmp1,'means')$tables$`delta:interval`),main="Power (J/BI) variation",xlab="Delta", ylab="Interval (ms)"))
	print(levelplot(as.table(model.tables(tmp2,'means')$tables$`delta:interval`),main="Power (J/s) variation",xlab="Delta", ylab="Interval (ms)"))
	print(levelplot(as.table(model.tables(tmp3,'means')$tables$`delta:interval`),main="Slowdown (%) variation",xlab="Delta", ylab="Interval (ms)"))
	dev.off(dev.cur())
	1
}
for(i in 1:length(workloads)){
	print.aov.wrk(workloads[i]);
}

