%generate trial tables for dual-task experiment - original stimulus set
%with only 28 videos

%we want to generate 1 set of trial tables, containing all the stimuli for
%each of the 3 dual-task condititions, and repeat that twice for the two
%instruction sets.

rng('shuffle');

Ndts = 3;   %number of dual-task types / blocks

%first col = prime/interfere, second col = correct/incorrect
stimtype = [0 0;
            0 1;
            1 0;
            1 1
           ];

NStimP = 28;
NStimT = 24;
NVids = 28;

DTstimTime = 2500;
DTwaitTime = DTstimTime+800;

Stim = {
         [ 0     18       6;
           1     15       0;
           2      4      22;
           3      2      27;
         ];
         [ 4     19       2;
           5     21       7;
           6     24      13;
           7     16      11;
         ];
         [ 8      0      18      24;
           9     25      13      19;
          10     15       9       6;
          11      2       4      13;
         ];
         [12     25      18      12;
          13     10      22       6;
          14      0      16       4;
          15     19       2       6;
         ];
         [16      8      25      18;
          17     25      15       5;
          18     22       0       8;
          19      0       5      25;
         ];
         [20      0        3       12       16;
          21      7       18       24       12;
          22     16       25       20       11;
          23      9       15        2        7;
         ];
%          [24     25       21        6        5;
%           25     18       24        7       11;
%           26     11        4       22       15;
%           27     13        9        2       27;
%          ];
      };

CStim = [ 0:3; 
          4:7; 
          8:11;
         12:15;
         16:19;
         20:23
        ];

for b = 1:2 %repeat twice with same stimuli
    for idts = 1:Ndts %for each context
        TrTbl{b,idts} = [];
        TrTblP{b,idts} = [];
    end
end


%of the trials in the set, we want half to be priming and half to be
%  interfering, and also half to be "correct" and half to be "incorrect"
%since we only have 4 stimuli, we will have to use all the stimuli in each
%  condition and bank on the fact that most people won't be able to
%  memorize the stimuli (i.e., no order effect).


for istim = 1:length(Stim)
    
    for ctx = 1:2 %for 2 context conditions
        
        
        %*****posture*****
        idts = 1; 
        
        %randomize the stimulus order
        iset = randperm(size(Stim{istim},1));
        
        for i = 1:length(iset)
            
            if stimtype(iset(i),1) == 0 %if we are priming
                stim = Stim{istim}(iset(i),2:end);
                stim = stim(randperm(length(stim)));
                stim = stim(1);
            else %if we are interfering
                stim = setxor([0:NStimP-1],Stim{istim}(iset(i),2:end));
                stim = stim(randperm(length(stim)));
                stim = stim(1);
            end

            rstim1 = stim(1)+NStimP;
            rstim2 = setxor([0:NStimP-1],Stim{istim}(iset(i),2:end));
            rstim2 = rstim2(randperm(length(rstim2)));
            rstim2 = rstim2(1)+NStimP;

            if stimtype(iset(i),2) == 1 %if resp is "left"
                respstim = [rstim1 rstim2];
                cresp = 1;
            else %resp is "right"
                respstim = [rstim2 rstim1];
                cresp = 2;
            end
            
            TrTbl{ctx,idts} = [TrTbl{ctx,idts};
                               1 Stim{istim}(iset(i),1) idts stimtype(iset(i),1) stim respstim cresp DTstimTime DTwaitTime 500 10000];
            
        end
        
        
        %*****trajectory*****
        idts = 2; 
        
        %randomize the stimulus order
        iset = randperm(size(Stim{istim},1));
        
        for i = 1:length(iset)
            
            if stimtype(iset(i),1) == 0 %if we are priming
                stim = Stim{istim}(iset(i),1);
            else %if we are interfering
                stim = setxor([0:NStimT-1],Stim{istim}(iset(i),1));
                stim = stim(randperm(length(stim)));
                stim = stim(1);
            end
            
            rstim1 = stim + NStimT;
            rstim2 = setxor([0:NStimT-1],[Stim{istim}(iset(i),1) stim]);
            rstim2 = rstim2(randperm(length(rstim2)));
            rstim2 = rstim2(1) + NStimT;
            
            if stimtype(iset(i),2) == 1 %if resp is "left"
                respstim = [rstim1 rstim2];
                cresp = 1;
            else %resp is "right"
                respstim = [rstim2 rstim1];
                cresp = 2;
            end
            
            TrTbl{ctx,idts} = [TrTbl{ctx,idts};
                               1 Stim{istim}(iset(i),1) idts stimtype(iset(i),1) stim respstim cresp DTstimTime DTwaitTime 500 10000];
            
        end
        
        
        
        %*****control*****
        idts = 3;
        
        %randomize the stimulus order
        iset = randperm(size(Stim{istim},1));
        
        for i = 1:length(iset)
            
            %pick a random row and column
            r = randperm(size(CStim,1));
            r1 = r(1);
            r = randperm(size(CStim,2));
            r2 = r(1);
            s1 = CStim(r1,r2);
            s2 = setxor(CStim(:,r2),s1);
            s2 = s2(randperm(length(s2)));
            %s3 = setxor(CStim(r1,:),s1); %pick a stim of the same color but different category
            %s3 = s3(randperm(length(s3)));
            s3 = setxor([0:max(CStim(:))],CStim(:,r2));  %pick any other stim not in the same column
            s3 = s3(randperm(length(s3)));

            stim = s1;
            rstim1 = s2(1);
            rstim2 = s3(1);
                
            if stimtype(iset(i),2) == 1 %if resp is "left"
                respstim = [rstim1 rstim2];
                cresp = 1;
            else %resp is "right"
                respstim = [rstim2 rstim1];
                cresp = 2;
            end
            
            TrTbl{ctx,idts} = [TrTbl{ctx,idts};
                               1 Stim{istim}(iset(i),1) idts 0 stim respstim cresp DTstimTime DTwaitTime 500 10000];
            
        end
    end
end


dttype = ['p';'t';'c'];
cntxt = ['body';'traj'];

trfilepath = '..\TrialTables\';


for a = 1:size(TrTbl,1)
    for b = 1:size(TrTbl,2)
        fid = fopen(fullfile(trfilepath,sprintf('dt%s_%c.txt',cntxt(a,:),dttype(b))),'wt');
        
        for c = 1:size(TrTbl{a,b},1)
            for d = 1:size(TrTbl{a,b},2)
                fprintf(fid,'%d ',TrTbl{a,b}(c,d));
            end
            fprintf(fid,'\n');
        end
        
        fclose(fid);
    end
end





%%
%make single-task trial tables

fid = fopen(fullfile(trfilepath,sprintf('im_practice.txt')),'wt');

%imitation alone
for a = 0:3
    
    nvid = NVids + a;
    
    fprintf(fid,'0 %d -1 -1 -1 -1 -1 -1 -1 -1 500 10000\n',nvid);
    
end
fclose(fid);


%secondary-task alone

%*****posture*****

fid = fopen(fullfile(trfilepath,sprintf('st_%c.txt',dttype(1))),'wt');

%randomize the stimulus order
randstim = randperm(NStimP)-1;

for a = 1:20

    stim = randstim(a);
    
    rstim1 = stim + NStimP;
    rstim2 = setxor([0:NStimP-1],stim);
    rstim2 = rstim2(randperm(length(rstim2)));
    rstim2 = rstim2(1)+NStimP;
    cresp = 1;
    
    if (rand > 0.5)
        tmp = rstim1;
        rstim1 = rstim2;
        rstim2 = tmp;
        cresp = 2;
    end
    
    fprintf(fid,'2 -1 1 -1 %d %d %d %d %d %d 500 10000\n',stim,rstim1,rstim2,cresp,DTstimTime,DTwaitTime);
    
end

fclose(fid);


%*****trajectory*****

fid = fopen(fullfile(trfilepath,sprintf('st_%c.txt',dttype(2))),'wt');

%randomize the stimulus order
randstim = randperm(NStimT)-1;

for a = 1:20

    stim = randstim(a);
    
    rstim1 = stim + NStimT;
    rstim2 = setxor([0:NStimT-1],stim);
    rstim2 = rstim2(randperm(length(rstim2)));
    rstim2 = rstim2(1)+NStimT;
    cresp = 1;
    
    if (rand > 0.5)
        tmp = rstim1;
        rstim1 = rstim2;
        rstim2 = tmp;
        cresp = 2;
    end
    
    fprintf(fid,'2 -1 2 -1 %d %d %d %d %d %d 500 10000\n',stim,rstim1,rstim2,cresp,DTstimTime,DTwaitTime);
    
end

fclose(fid);



%*****control*****

fid = fopen(fullfile(trfilepath,sprintf('st_%c.txt',dttype(3))),'wt');

%randomize the stimulus order

for a = 1:20
    
    %pick a random row and column
    r = randperm(size(CStim,1));
    r1 = r(1);
    r = randperm(size(CStim,2));
    r2 = r(1);
    s1 = CStim(r1,r2);
    s2 = setxor(CStim(:,r2),s1);
    s2 = s2(randperm(length(s2)));
    %s3 = setxor(CStim(r1,:),s1);  %pick a stim of the same color but different category
    %s3 = s3(randperm(length(s3)));
    s3 = setxor([0:max(CStim(:))],CStim(:,r2));  %pick any other stim not in the same column
    s3 = s3(randperm(length(s3)));
    
    stim = s1;
    rstim1 = s2(1);
    rstim2 = s3(1);

    if rand < 0.5
        respstim = [rstim1 rstim2];
        cresp = 1;
    else %resp is "right"
        respstim = [rstim2 rstim1];
        cresp = 2;
    end
    
    fprintf(fid,'2 -1 3 -1 %d %d %d %d %d %d 500 10000\n',stim,respstim(1),respstim(2),cresp,DTstimTime,DTwaitTime);
    
end

fclose(fid);


%make dual-task practice trials


for reps = 1:2
    
    randstimp = randperm(NStimP)-1;
    randstimt = randperm(NStimT)-1;
    
    for istim = 0:3
        
        nvid = NVids + istim;
        
        %posture
        stim = randstimp(a);
        
        rstim1 = stim + NStimP;
        rstim2 = setxor([0:NStimP-1],stim);
        rstim2 = rstim2(randperm(length(rstim2)));
        rstim2 = rstim2(1)+NStimP;
        cresp = 1;
        
        if (rand > 0.5)
            tmp = rstim1;
            rstim1 = rstim2;
            rstim2 = tmp;
            cresp = 2;
        end
        
        respstim = [rstim1 rstim2];
        
        TrTblP{reps,1} = [TrTblP{reps,1};
                         1 nvid 1 0 stim respstim cresp DTstimTime DTwaitTime 500 10000];
        
        
        %trajectory
        stim = randstimt(a);
        
        rstim1 = stim + NStimT;
        rstim2 = setxor([0:NStimT-1],stim);
        rstim2 = rstim2(randperm(length(rstim2)));
        rstim2 = rstim2(1)+NStimT;
        cresp = 1;
        
        if (rand > 0.5)
            tmp = rstim1;
            rstim1 = rstim2;
            rstim2 = tmp;
            cresp = 2;
        end
        
        respstim = [rstim1 rstim2];
        
        
        TrTblP{reps,2} = [TrTblP{reps,2};
                         1 nvid 2 0 stim respstim cresp DTstimTime DTwaitTime 500 10000];
        
        
        %control
        %pick a random row and column
        r = randperm(size(CStim,1));
        r1 = r(1);
        r = randperm(size(CStim,2));
        r2 = r(1);
        s1 = CStim(r1,r2);
        s2 = setxor(CStim(:,r2),s1);
        s2 = s2(randperm(length(s2)));
        %s3 = setxor(CStim(r1,:),s1);  %pick a stim of the same color but different category
        %s3 = s3(randperm(length(s3)));
        s3 = setxor([0:max(CStim(:))],CStim(:,r2));  %pick any other stim not in the same column
        s3 = s3(randperm(length(s3)));
        
        stim = s1;
        rstim1 = s2(1);
        rstim2 = s3(1);
        
        if rand < 0.5
            respstim = [rstim1 rstim2];
            cresp = 1;
        else %resp is "right"
            respstim = [rstim2 rstim1];
            cresp = 2;
        end
        
        TrTblP{reps,3} = [TrTblP{reps,3};
                         1 nvid 3 0 stim respstim cresp DTstimTime DTwaitTime 500 10000];

    end
end



for a = 1:size(TrTblP,1)
    for b = 1:size(TrTblP,2)
        fid = fopen(fullfile(trfilepath,sprintf('dtpractice_%s_%c.txt',cntxt(a,:),dttype(b))),'wt');
        
        for c = 1:size(TrTblP{a,b},1)
            for d = 1:size(TrTblP{a,b},2)
                fprintf(fid,'%d ',TrTblP{a,b}(c,d));
            end
            fprintf(fid,'\n');
        end
        
        fclose(fid);
    end
end
