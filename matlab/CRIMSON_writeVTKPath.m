function CRIMSON_writeVTKPath( pathAndContours, fileName )
% Writes a vessel path and contours as polylines in a VTK file
% See CRIMSON_readVTKPath for expected format

outFile = fopen(fileName, 'w');

% Write header
fprintf(outFile, '# vtk DataFile Version 3.0\n');
fprintf(outFile, 'CRIMSON vessel path output from MATLAB\n');
fprintf(outFile, 'ASCII\nDATASET POLYDATA\n');

polyLines = [pathAndContours.vesselPath, pathAndContours.contours];

%%% Write points
% Write point info header
totalNumberOfPoints = sum(cellfun(@(x) size(x, 2),polyLines));
fprintf(outFile, 'POINTS %i float\n', totalNumberOfPoints);

% Write point coordinates
cellfun(@(polyLine) fprintf(outFile, '%f %f %f\n', polyLine), polyLines);

%%% Write cells
% Write cell info header
fprintf(outFile, 'LINES %i %i\n', length(polyLines), totalNumberOfPoints + length(polyLines));

nextId = 0;
for i=1:length(polyLines)
    firstId = nextId;
    nextId = nextId + size(polyLines{i}, 2);
    % Write number of IDs
    fprintf(outFile, '%i ', size(polyLines{i}, 2));
    % Write IDs
    fprintf(outFile, '%i ', firstId:(nextId - 1));
    fprintf(outFile, '\n');
end

fclose(outFile);

