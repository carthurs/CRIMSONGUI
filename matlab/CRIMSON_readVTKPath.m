function [ pathAndContours ] = CRIMSON_readVTKPath( fileName )
% Reads a vessel path and contours described as polylines in a VTK file
% Output is a structure containing members:
%  - vesselPath - the coordinates of points defining the vessel path
%  (matrix 3 x n)
%  - contours - the cell array, where each cell contains the coordinates of
%  points defining each contour (matrix 3 x n_i). The contours are closed
%  so the last point coincides with the first one

inFile = fopen(fileName)

% Skip header
fgetl(inFile);
fgetl(inFile);

% Confirm that the dataset is ASCII
if ~strcmp(fgetl(inFile), 'ASCII')
    error('File "%s" contains non-ASCII data. Aborting read.', fileName)
end

if ~strcmp(fgetl(inFile), 'DATASET POLYDATA')
    error('File "%s" contains non-polydata data. Aborting read.', fileName)
end

nTuples = fscanf(inFile, 'POINTS %li %*s\n', 2);

if length(nTuples) == 0
    error('Failed to find POINTS data array in file "%s". Aborting read.', fileName)
end

pointCoordinates = fscanf(inFile, '%f\n', [3, nTuples]); % Each column contains a point coordinate

if size(pointCoordinates, 2) ~= nTuples
    error('Failed to read point coordinates from file "%s". Aborting read.', fileName)
end

nLines = fscanf(inFile, 'LINES %li %*li', 2);

if length(nLines) == 0
    error('Failed to find LINES in file "%s". Aborting read.')
end


% Read the polylines
polyLines = {};

for lineId = 1:nLines
    nPointIds = fscanf(inFile, '%li', 1);
    polyLinePointIds = fscanf(inFile, '%li', nPointIds) + 1;
    
    polyLines{lineId} = zeros(3, nPointIds);
    
    for i = 1:length(polyLinePointIds)
        polyLines{lineId}(:,i) = pointCoordinates(:,polyLinePointIds(i));
    end
end

fclose(inFile);

pathAndContours.vesselPath = polyLines{1};
pathAndContours.contours = polyLines(2:end);



    
    



