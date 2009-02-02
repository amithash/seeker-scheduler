%This code takes some file from command line
function [] = plot_bench(infile,outfig1,outfig2,device)
a = load(infile);
new1=figure;
set(new1,"visible", "off");
subplot(3,1,1);
plot(a(:,1),a(:,4));
xlabel('Instructions','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
ylabel('IPC','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
subplot(3,1,2);
plot(a(:,1),a(:,3));
xlabel('Instructions','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
ylabel('CPU','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
subplot(3,1,3);
plot(a(:,1),a(:,3));
xlabel('Instructions','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
ylabel('CPU','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
subplot(3,1,3);
plot(a(:,1),a(:,6),'b');
xlabel('Instructions','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
ylabel('CPU State','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
dev = strcat('-d',device);
print('new1','-djpeg',outfig1);
close all;
new2 = figure;
set(new2,"visible", "off");
subplot(2,1,1);
hist(a(:,4));
xlabel('IPC','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
ylabel('Frequency','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
subplot(2,1,2);
hist(a(:,6));
xlabel('CPU State','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
ylabel('frequency','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
print('new1','-djpeg',outfig2);
close all;

