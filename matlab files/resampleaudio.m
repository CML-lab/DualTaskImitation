%resample all the audio files

[fnames,fpath] = uigetfile('*.wav','Multiselect','On');

if ~iscell(fnames)
    fnames = {fnames};
end

fs1 = 22050;  %44100?

for a = 1:length(fnames)
    [y,fs] = audioread([fpath fnames{a}]);
    sound(y,fs);
    pause(3);
    [P,Q] = rat(fs1/fs);  
    
    ynew = resample(y,P,Q);
    sound(ynew,fs1);
    
    audiowrite([fpath fnames{a}],ynew,fs1);
    pause(3);
    
end