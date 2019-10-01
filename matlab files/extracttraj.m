%load and clip trajectories for dual-task experiment

%load the data
[fname,fpath] = uigetfile('*.dat','Select data files to analyze','Multiselect','on');

if ~iscell(fname)
    fname = {fname};
end

for a = 1:length(fname)
    [data,params] = trakSTARload([fpath fname{a}]);
    
    trial(a).dat = data{1};
    
end
clear data params;

vthresh = .01;
vthreshMin = .0025;
tcombine = 5;      %duration within which sections will be combined


%extract the trajectory from each trial
for tr = 1:length(trial)
    
    %find the velocity peaks
    dt = mean(diff(trial(tr).dat.TrackerTime)); %sampling interval, sec
    trial(tr).dat.VelX = gradient(sgolayfilt(trial(tr).dat.HandX,2,min([19,2*floor((length(trial(tr).dat.HandX)-2)/2)+1])),dt);
    trial(tr).dat.VelY = gradient(sgolayfilt(trial(tr).dat.HandY,2,min([19,2*floor((length(trial(tr).dat.HandY)-2)/2)+1])),dt);
    trial(tr).dat.VelZ = gradient(sgolayfilt(trial(tr).dat.HandZ,2,min([19,2*floor((length(trial(tr).dat.HandZ)-2)/2)+1])),dt);
    
    %find the indices of the velocity peaks
    vel3d = sqrt(trial(tr).dat.VelX.^2 + trial(tr).dat.VelY.^2 + trial(tr).dat.VelZ.^2);
    vless = vel3d > vthresh;
    veliless = find(vless == 1);
    velidless = find(diff([0; veliless])>1); %now i have the indices into veli of all the starts of all reaches exceeding vthreshMax
    if isempty(velidless)
        velidless = veliless(1);
    end
    velidless = sort([velidless; velidless-1]);
    velidless = [velidless(2:end); length(veliless)];
    veliless = veliless(velidless); %now i have the start and end indices of all reaches exceeding vthreshMax
    for a = 1:length(veliless)-1
        if abs(veliless(a+1)-veliless(a)) < tcombine %if the start/end marks are too close, throw them out! alternatively maybe merge them?
            veliless(a) = NaN;
            veliless(a+1) = NaN;
        end
    end
    veliless = veliless(~isnan(veliless));
    if mod(length(veliless),2) ~= 0
        for a = 2:length(veliless)-1
            if (all(vel3d(veliless(a-1)+1:veliless(a)-1) > vthresh) && all(vel3d(veliless(a)+1:veliless(a+1)-1) > vthresh)) || (all(vel3d(veliless(a-1)+1:veliless(a)-1) < vthresh) && all(vel3d(veliless(a)+1:veliless(a+1)-1) < vthresh))
                veliless = sort([veliless; veliless(a)]);
                break;
            end
        end
    end
    if mod(length(veliless),2) ~= 0
        if length(veliless) > 3
            ind{1} = [veliless(1) veliless(end)];
            ind{2} = [veliless(2:end-1)];
        elseif length(veliless) > 2
            ind{1} = [veliless(1) veliless(end)];
            ind{2} = [];
        else
            ind = cell(2);
        end
        return;
    end
    
    veli = [1; veliless; size(vel3d,1)];

    i = 1;
    clear velimin;
    for a = 1:2:length(veli)
        vsection = vel3d(veli(a):veli(a+1));
        [~,imin] = min(vsection);
        if (a < length(veli)-1) %if we're not on the last pair, find a start
            iminfirst = find(vsection(imin:end) > vthreshMin,1,'first');
            if isempty(iminfirst)
                [~,iminfirst] = min(vsection);
            end
            velimin(i,2) = veli(a)+ imin + iminfirst-1;
        else
            velimin(i,2) = NaN;
        end
        
        if (a > 1)
            iminlast = find(vsection(1:imin) > vthreshMin,1,'last');
            if isempty(iminlast)
                [~,iminlast] = min(vsection);
            end
            velimin(i,1) = veli(a)+iminlast-1;
        else
            velimin(i,1) = NaN;
        end
        i = i+1;
        
    end
    velimin = [velimin(1:end-1,2) velimin(2:end,1)]; %these are the indices of velocity starts and ends
    for a = 2:size(velimin,1)
        if abs(velimin(a,1)-velimin(a-1,2)) < (tcombine/3) %if successive start/end marks are too close, throw them out! alternatively maybe merge them?
            velimin(a,1) = NaN;
            velimin(a-1,2) = NaN;
        end
    end
    tmp = velimin;
    velimin = tmp(~isnan(tmp(:,1)),1);
    velimin(:,2) = tmp(~isnan(tmp(:,2)),2);
    
    inds = reshape(velimin,1,[]);
    
    %verify the indices
    trial(tr).dat.inds = markdata3d([trial(tr).dat.HandX,trial(tr).dat.HandY,trial(tr).dat.HandZ],[trial(tr).dat.VelX,trial(tr).dat.VelY,trial(tr).dat.VelZ],vel3d,5000,inds,1);
    
    trial(tr).x = trial(tr).dat.HandX(trial(tr).dat.inds(1):trial(tr).dat.inds(end));
    trial(tr).y = trial(tr).dat.HandY(trial(tr).dat.inds(1):trial(tr).dat.inds(end));
    trial(tr).z = trial(tr).dat.HandZ(trial(tr).dat.inds(1):trial(tr).dat.inds(end));
    
end

xmin = inf;
xmax = -inf;
ymin = inf;
ymax = -inf;
zmin = inf;
zmax = -inf;

%smooth the trajectories we will down-sample the curve to get rid of
%smaller oscillations
for a = 1:length(trial)
    
    %note, the potential issue with smoothing the curves here
    %  is that potentially you run into problems when the
    %  angles sharply change.  So you have to be careful not to
    %  downsample too much?

    dn = 10;
    t1 = [1 1:length(trial(a).x)/dn:length(trial(a).x)];
    if t1(end) ~= length(trial(a).x) %101
        t1(end) = length(trial(a).x); %101;
    end
    t1(end+1) = length(trial(a).x);
    values1 = spline([1:length(trial(a).x)],[trial(a).x,trial(a).y,trial(a).z]',t1);
    
    %use the b-spline fit for interpolation to smooth out the data better...  we want a cubic spline!
    values2 = spcrv(values1,4,length(trial(a).x)/2);
                
	trial(a).smoothfit = values2(:,2:end-1)';
    
    if max(trial(a).smoothfit(:,1)) > xmax
        xmax = max(trial(a).smoothfit(:,1));
    end
    if min(trial(a).smoothfit(:,1)) < xmin
        xmin = min(trial(a).smoothfit(:,1));
    end
    if max(trial(a).smoothfit(:,2)) > ymax
        ymax = max(trial(a).smoothfit(:,2));
    end
    if min(trial(a).smoothfit(:,2)) < ymin
        ymin = min(trial(a).smoothfit(:,2));
    end
    if max(trial(a).smoothfit(:,3)) > zmax
        zmax = max(trial(a).smoothfit(:,3));
    end
    if min(trial(a).smoothfit(:,3)) < zmin
        zmin = min(trial(a).smoothfit(:,3));
    end
    
end

xmin = 0;
xmax = 0.04;
ymin = -0.02;
ymax = 0.02;
zmin = -0.01;
zmax = 0.03;



az = 90;
el = 0;
close all;

%plot the data
for a = 1:length(trial)
    figure(a);
    h = plot3(trial(a).smoothfit(:,1),trial(a).smoothfit(:,2),-trial(a).smoothfit(:,3),'b-');
    set(h,'LineWidth',2);
    hold on
    h = plot3(trial(a).smoothfit(1,1),trial(a).smoothfit(1,2),-trial(a).smoothfit(1,3),'bo');
    hold off;
    set(h,'LineWidth',2,'Color','b','MarkerFaceColor','b','MarkerSize',10);
    view(az,el);
    set(gca,'visible','off')
    set(gca,'xlim',[xmin xmax],'ylim',[ymin ymax],'zlim',[zmin zmax]);
    
end


%save the data
save([fpath 'extracted_trajectories.mat'],'trial');

