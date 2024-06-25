% Run this script if you want to run the model without openplc
simInput = struct("pump", 1, "valve", 1);
isPaused = false;
isLogging = false;

if isLogging
    outputFile = fopen("matlabOutput.json", "wt");
    fprintf(outputFile, "[");
end
