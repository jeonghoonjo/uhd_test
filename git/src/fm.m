%fc = 4000;         %carrier freq
%fdev = 75000;    %frequency deviation
fs = 5000000;       %sampling freq
f = 1000;           %input signal freq
t = (0 : 1/fs : 0.001-1/fs);

x = cos(2*pi*f*t);  %input signal
%y = fmmod(x, fc, fs, fdev);
%complex_y = complex(y);


fileID = fopen('test.bin', 'w');
fwrite(fileID, x, 'float');
fclose(fileID);



