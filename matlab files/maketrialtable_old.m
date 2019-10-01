%generate trial tables for dual-task experiment

%we want to generate 2 pairs of sets of trial tables:
%for each set, half the trials will be included
%each pair will have the same trial split, so we can run 1 subject with
%those same trials with copy-body instructions, and 1 with copy-trajectory
%instructions

Nreps = 1;  %number of reps of each stim per block
Ndts = 3;   %number of dual-task types / blocks

%first col = prime/interfere, second col = correct/incorrect
stimtype = [0 0;
            0 1;
            1 0;
            1 1
           ];

Stim = {
         [ 0      0      1;
           1      2      3;
           2      4      5;
           3      6      7;
         ];
         [ 4      8      9;
           5     10     11;
           6     12     13;
           7     14     15;
         ];
         [ 8     16     17     18;
           9     19     20     21;
          10     22     23     24;
          11     25     26     27;
         ];
         [12     28     29     30;
          13     31     32     33;
          14     34     35     36;
          15     37     38     39;
         ];
         [16     40     41     42;
          17     43     44     45;
          18     46     47     48;
          19     49     50     51;
         ];
         [20     52     53     54     55;
          21     56     57     58     59;
          22     60     61     62     63;
          23     64     65     66     67;
         ];
         [24     68     69     70     71;
          25     72     73     74     75;
          26     76     77     78     79;
          27     80     81     82     83;
         ];
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
                if length(stim) > 3
                    r = rand;
                    if r > 2/3
                        stim = stim(3:4);
                    elseif r > 1/3
                        stim = stim(2:3);
                    else
                        stim = stim(1:2);
                    end
                elseif length(stim) > 2
                    if rand > 1/2
                        stim = stim(2:3);
                    else
                        stim = stim(1:2);
                    end
                end
            else %if we are interfering
                stim = setxor([0:83],Stim{istim}(iset(i),2:end));
                stim = stim(randperm(length(stim)));
                stim = stim(1:2);
            end
            
            if stimtype(iset(i),2) == 1 %if resp is "yes"
                if rand > 0.5
                    respstim = stim(1);
                else
                    respstim = stim(2);
                end
            else %resp is "no"
                respstim = setxor([0:83],[stim Stim{istim}(iset(i),2:end)]);
                respstim = respstim(randperm(length(respstim)));
                respstim = respstim(1);
            end
            
            TrTbl{ctx,idts} = [TrTbl{ctx,idts};
                               1 Stim{istim}(iset(i),1) idts stimtype(iset(i),1) stim respstim stimtype(iset(i),2) 1500 1500 500 10000];
            
        end
        
        
        %*****trajectory*****
        idts = 2; 
        
        %randomize the stimulus order
        iset = randperm(size(Stim{istim},1));
        
        for i = 1:length(iset)
            
            if stimtype(iset(i),1) == 0 %if we are priming
                stim = [Stim{istim}(iset(i),1) -1];
            else %if we are interfering
                stim = setxor([0:27],Stim{istim}(iset(i),1));
                stim = stim(randperm(length(stim)));
                stim = [stim(1) -1];
            end
            
            if stimtype(iset(i),2) == 1 %if resp is "yes"
                    respstim = stim(1);
            else %resp is "no"
                respstim = setxor([0:27],[stim Stim{istim}(iset(i),1)]);
                respstim = respstim(randperm(length(respstim)));
                respstim = respstim(1);
            end
            respstim = respstim+28;
            
            TrTbl{ctx,idts} = [TrTbl{ctx,idts};
                               1 Stim{istim}(iset(i),1) idts stimtype(iset(i),1) stim respstim stimtype(iset(i),2) 1500 1500 500 10000];
            
        end
        
        
        
        %*****control*****
        idts = 3;
        
        %randomize the stimulus order
        iset = randperm(size(Stim{istim},1));
        
        for i = 1:length(iset)
            
            if rand > 0.5 %do stim category type
                
                r = randperm(size(CStim,2));
                r1 = r(1);
                r2 = r(2);
                r3 = r(3);
                s1 = CStim(:,r1);
                s1 = s1(randperm(length(s1)));
                s2 = CStim(:,r2);
                s2 = s2(randperm(length(s2)));

                if stimtype(iset(i),2) == 1 %if resp is "yes"
                    if rand > 0.5
                        respstim = s1(2);
                    else
                        respstim = s2(2);
                    end
                else %resp is "no"
                    respstim = CStim(:,r3);
                    respstim = respstim(randperm(length(respstim)));
                    respstim = respstim(1);
                end
                stim = [s1(1) s2(1)];
                
            else %do stim color type
            
                r = randperm(size(CStim,1));
                r1 = r(1);
                r2 = r(2);
                r3 = r(3);
                s1 = CStim(r1,:);
                s1 = s1(randperm(length(s1)));
                s2 = CStim(r2,:);
                s2 = s2(randperm(length(s2)));

                if stimtype(iset(i),2) == 1 %if resp is "yes"
                    if rand > 0.5
                        respstim = s1(2);
                    else
                        respstim = s2(2);
                    end
                else %resp is "no"
                    respstim = CStim(r3,:);
                    respstim = respstim(randperm(length(respstim)));
                    respstim = respstim(1);
                end
                stim = [s1(1) s2(1)];
            end
            
            TrTbl{ctx,idts} = [TrTbl{ctx,idts};
                               1 Stim{istim}(iset(i),1) idts 0 stim respstim stimtype(iset(i),2) 1500 1500 500 10000];
            
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
