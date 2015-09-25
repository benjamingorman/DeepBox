local os = require("os")
local string = require("string")

-- Colours:
backgroundColor = {39,40,34,255}
dotColor = {28,28,28,255}
edgeFreeColor = {55,56,48,255}
edgeHoverColor = {159,168,168,255}
edgeTakenColor = {253,151,31,255}
boxTakenColor = {253,151,31,60}
fontColor = {166,226,46,255}

gridStartX = 100
gridStartY = 100
gridSpacing = 100

textStartX = gridStartX
textStartY = gridStartY - 60

numDots = 45
dotRadius = 12 
dotSegments = 300

numEdges = 72
edgeWidth = 10

numBoxes = 28

dotIndices = {
    [0]={0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, {6,0}, {7,0}, {8,0},
    {0,1}, {1,1}, {2,1}, {3,1}, {4,1}, {5,1}, {6,1}, {7,1}, {8,1},
    {0,2}, {1,2}, {2,2}, {3,2}, {4,2}, {5,2}, {6,2}, {7,2}, {8,2},
    {1,3}, {2,3}, {3,3}, {5,3}, {6,3}, {7,3},
    {1,4}, {2,4}, {3,4}, {5,4}, {6,4}, {7,4},
    {1,5}, {2,5}, {3,5}, {5,5}, {6,5}, {7,5}
}

edgeDots = {
    [0]={0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,8}, {0,9},{1,10},{2,11},{3,12},{4,13},{5,14},{6,15},{7,16},{8,17}, {9,10},{10,11},{11,12},{12,13},{13,14},{14,15},{15,16},{16,17}, {9,18},{10,19},{11,20},{12,21},{13,22},{14,23},{15,24},{16,25},{17,26}, {18,19},{19,20},{20,21},{21,22},{22,23},{23,24},{24,25},{25,26}, {19,27},{20,28},{21,29},{23,30},{24,31},{25,32}, {27,28},{28,29},{30,31},{31,32}, {27,33},{28,34},{29,35},{30,36},{31,37},{32,38}, {33,34},{34,35},{36,37},{37,38}, {33,39},{34,40},{35,41},{36,42},{37,43},{38,44}, {39,40},{40,41},{42,43},{43,44}
}

boxEdges = {
    [0]={0,8,9,17},    {1,9,10,18},   {2,10,11,19},
    {3,11,12,20},  {4,12,13,21},  {5,13,14,22},
    {6,14,15,23},  {7,15,16,24},  {17,25,26,34},
    {18,26,27,35}, {19,27,28,36}, {20,28,29,37},
    {21,29,30,38}, {22,30,31,39}, {23,31,32,40},
    {24,32,33,41}, {35,42,43,48}, {36,43,44,49},
    {39,45,46,50}, {40,46,47,51}, {48,52,53,58},
    {49,53,54,59}, {50,55,56,60}, {51,56,57,61},
    {58,62,63,68}, {59,63,64,69}, {60,65,66,70},
    {61,66,67,71}
}

edges = {}
boxes = {}

hoveredEdge = nil

saveFileName = "position.dbl"

function getDotPosition(i)
    x = gridStartX + dotIndices[i][1] * gridSpacing
    y = gridStartY + dotIndices[i][2] * gridSpacing
    return x,y
end

function isMouseOverEdge(edge)
    local mx, my = love.mouse.getPosition()
    local x1, y1 = getDotPosition(edge.from)
    local x2, y2 = getDotPosition(edge.to)
    local min_x, max_x = x1 - edgeWidth/2, x2 + edgeWidth/2
    local min_y, max_y = y1 - edgeWidth/2, y2 + edgeWidth/2

    if mx >= min_x and mx <= max_x and my >= min_y and my <= max_y then
        return true
    else
        return false
    end
end

function isEdgeTaken(edge)
    return edge.taken == true
end

function isBoxTaken(box)
    return box.taken == true
end

function chooseMove(e)
    print("Choosing edge ", e.index)
    edges[e.index].taken = true
    updateBoxes()
end

-- MAIN
function newGame()
    -- Init edges
    for i=0, numEdges-1 do
        local dots = edgeDots[i]
        edges[i] = {index=i, from=dots[1], to=dots[2], taken=false}
    end

    -- Init boxes
    for i=0, numBoxes-1 do
        boxes[i] = {taken=false}
    end
end

function updateBoxes()
    for i=0, numBoxes-1 do
        if not isBoxTaken(boxes[i]) then
            local takenEdgeCount = 0
            for _, edgeIndex in ipairs(boxEdges[i]) do
                if isEdgeTaken(edges[edgeIndex]) then
                    takenEdgeCount = takenEdgeCount + 1
                end
            end

            if takenEdgeCount == 4 then
                print("Box " .. i .. " was captured!")
                boxes[i].taken = true
            end
        end
    end
end

function saveState()
    -- Writes the state to a file as a string
    print("Saving game state to file '" .. saveFileName .. "' in " .. love.filesystem.getSaveDirectory())
    local file = love.filesystem.newFile(saveFileName, "w")

    for i=0, numEdges-1 do
        if edges[i].taken then
            file:write("1")
        else
            file:write("0")
        end
    end

    file:write("\n")
    file:close()

    print("Finished writing to file.")
end

function loadState(fileName)
    local file = love.filesystem.newFile(fileName)
    file:open("r")
    local stateString = file:read()
    file:close()

    for i=1, numEdges do
        if stateString:sub(i,i) == "1" then
            print("Setting edge taken. Index " .. i)
            edges[i-1].taken = true
        else
            edges[i-1].taken = false
        end
    end
    updateBoxes()
end

function love.load()
    love.graphics.setLineWidth(edgeWidth)
    love.graphics.setBackgroundColor(backgroundColor)
    love.graphics.setFont(love.graphics.newFont(25))

    newGame()

    if arg[2] == "load" then
        loadState(arg[3])
    elseif arg[2] == "save" then
        saveFileName = arg[3]
    end
end

function love.update(dt)
    hoveredEdge = nil
    for i=0, numEdges-1 do
        edge = edges[i]
        if isMouseOverEdge(edge) then
            hoveredEdge = edge
            break
        end
    end

    if love.mouse.isDown("l") and hoveredEdge ~= nil and not isEdgeTaken(hoveredEdge) then
        chooseMove(hoveredEdge)
    end
end

function love.draw()
    -- Boxes
    for i=0, numBoxes-1 do
        local box = boxes[i]

        if box.taken then
            local x,y = getDotPosition(edges[boxEdges[i][1]].from)
            love.graphics.setColor(boxTakenColor)
            love.graphics.rectangle("fill", x, y, gridSpacing, gridSpacing)
        end
    end

    -- Edges
    for i=0, numEdges-1 do
        local edge = edges[i]
        local x1, y1 = getDotPosition(edge.from)
        local x2, y2 = getDotPosition(edge.to)

        if edge.taken then
            love.graphics.setColor(edgeTakenColor)
        elseif hoveredEdge == edge then
            love.graphics.setColor(edgeHoverColor)
        else
            love.graphics.setColor(edgeFreeColor)
        end
        
        love.graphics.line(x1, y1, x2, y2)
    end

    -- Dots
    for i=0, numDots-1 do
        local x,y = getDotPosition(i)
        love.graphics.setColor(dotColor)
        love.graphics.circle("fill", x, y, dotRadius, dotSegments)
    end
end

function love.keypressed(key, isrepeat)
    if key == "s" and isrepeat == false then
        saveState()
    end
end

