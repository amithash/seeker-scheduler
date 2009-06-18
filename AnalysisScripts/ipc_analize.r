a = read.table('123.tch', sep=',');
xl = c(0,2)
yl = c(0,2)
freqs = c(1100,1400,1700,2000,2200)
nor = 2200 / freqs
slope = freqs / 2200

#Plot parameters.
freqs_s = c('1100 MHz', '1400 MHz', '1700 MHz', '2000 MHz', '2200 MHz');
col_f = c('blue', 'green', 'orange', 'red', 'purple')
freqs_pch = c('+','+','+','+','+')
grid_col = "black"

jpeg('actual.jpg',width=1024,height=800)
plot(a$V9,a$V10,col=col_f[5],pch="+",xlim=xl,ylim=yl,axes=TRUE,xlab="Real IPC", ylab="Reference IPC",xaxs='i',yaxs='i')
par(new=TRUE);
plot(a$V7,a$V8,col=col_f[4],pch="+",xlim=xl,ylim=yl,axes=FALSE,xlab="", ylab="",xaxs='i',yaxs='i')
par(new=TRUE);
plot(a$V5,a$V6,col=col_f[3],pch="+",xlim=xl,ylim=yl,axes=FALSE,xlab="", ylab="",xaxs='i',yaxs='i')
par(new=TRUE);
plot(a$V3,a$V4,col=col_f[2],pch="+",xlim=xl,ylim=yl,axes=FALSE,xlab="", ylab="",xaxs='i',yaxs='i')
par(new=TRUE);
plot(a$V1,a$V2,col=col_f[1],pch="+",xlim=xl,ylim=yl,axes=FALSE,xlab="", ylab="",xaxs='i',yaxs='i')
par(new=TRUE);
abline(a=0,b=slope[1],col=col_f[1])
abline(a=0,b=slope[2],col=col_f[2])
abline(a=0,b=slope[3],col=col_f[3])
abline(a=0,b=slope[4],col=col_f[4])
abline(a=0,b=slope[5],col=col_f[5])

legend('topleft',freqs_s, col=col_f, pch=freqs_pch)
#par(new=TRUE)
#grid(col=grid_col);
dev.off();

#jpeg('normal.jpg',width=1024,height=800)
#plot(a$V1,a$V2 * nor[1],col=col_f[1],pch="+",xlim=xl,ylim=yl,axes=TRUE,xlab="Real IPC", ylab="Reference IPC")
#par(new=TRUE);
#plot(a$V3,a$V4 * nor[2],col=col_f[2],pch="+",xlim=xl,ylim=yl,axes=FALSE,xlab="", ylab="")
#par(new=TRUE);
#plot(a$V5,a$V6 * nor[3],col=col_f[3],pch="+",xlim=xl,ylim=yl,axes=FALSE,xlab="", ylab="")
#par(new=TRUE);
#plot(a$V7,a$V8 * nor[4],col=col_f[4],pch="+",xlim=xl,ylim=yl,axes=FALSE,xlab="", ylab="")
#par(new=TRUE);
#plot(a$V9,a$V10 * nor[5],col=col_f[5],pch="+",xlim=xl,ylim=yl,axes=FALSE,xlab="", ylab="")
#par(new=TRUE);
#legend('topleft',freqs_s, col=col_f, pch=freqs_pch)
#dev.off();
