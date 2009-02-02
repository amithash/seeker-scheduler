%This code takes some file from command line
function [x] = plot_mutate(infile_req,infile_giv,infile_pow,outfig,device,interval)
a1 = load(infile_req);
a2 = load(infile_giv);
a3 = load(infile_pow);
b1 = a1(:,2:end);
b2 = a2(:,2:end);
states = size(b1)(2);
leg = '';
for i = 1:states;
        leg = strvcat(leg,sprintf("State %d",i-1));
end

time = a1(:,1) .* interval;
new1=figure;
set (new1, "visible", "off");

subplot(3,1,1);
area(time,b1);
grid on;
xlabel('Interval','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
ylabel('CPUs Requested','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
legend(leg);
colormap summer;

subplot(3,1,2);
area(time,b2);
xlabel('Interval','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
ylabel('CPUs Given','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
legend(leg);
colormap summer;

subplot(3,1,3);
plot(a3(:,1),a3(:,2),'r');
xlabel('Time (Seconds)','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
ylabel('Power (Watts)','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
x = trapz(a3(:,1),a3(:,2));

dev = strcat('-d',device);
print('new1','-djpeg',outfig);
close all

