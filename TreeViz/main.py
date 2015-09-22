import flask
import sys
import re
import os
import json

TREE_FILE = sys.argv[1]

class MCTSTree:
    def __init__(self):
        self.tree = None

    def setDepth(self, tree, level):
        tree["depth"] = level
        newChildren = []
        for child in tree["children"]:
            newChildren.append(self.setDepth(child, level+1))

        tree["children"] = newChildren
        return tree

    def initFromString(self, s):
        self.tree = self.setDepth(json.loads(s), 0)

    def getTitle(self):
        return "MCTS Tree visualization"

    def getJsonData(self):
        nodes = []
        edges = []

        stack = []
        stack.append(self.tree)

        while len(stack) != 0:
            child = stack.pop()
            node = {}

            for key, item in child.items():
                if key != "children":
                    node[key] = item

            node["label"] = node["address"]
            node["id"] = node["address"]
            node["value"] = node["visits"]
            node["level"] = node["depth"] * 5

            if node["parent"] != "(nil)":
                edge = {}
                edge["from"] = node["parent"]
                edge["to"] = node["address"]
                edge["label"] = node["move"]
            
                edges.append(edge)

            nodes.append(node)
            for c in child["children"]:
                stack.append(c)

        return (json.dumps(nodes), json.dumps(edges))

app = flask.Flask(__name__)

@app.route('/')
def main():
    with open(TREE_FILE, "r") as f:
        tree = MCTSTree()
        tree.initFromString(f.read())

    (nodes, edges) = tree.getJsonData()
    print("nodes: " + nodes)
    print("edges: " + edges)

    return """<!DOCTYPE html>
<html>
    <head>
        <meta charset="UTF-8">
        <title>Script dependency visualization</title>
        <script type="text/javascript" src="https://cdnjs.cloudflare.com/ajax/libs/vis/4.8.1/vis.js"></script>
        <link rel="stylesheet" type="text/css" href="https://cdnjs.cloudflare.com/ajax/libs/vis/4.8.1/vis.css">
    </head>
    <body>
        <h1>{0}</h1>
        <div id="stateViz" style="white-space: pre; font-family: monospace; background: rgba(172, 167, 239, 120); width: 240px;"></div>
        <div id="network" style="position: absolute; top: 40px; bottom: 20px; width: 100%;"></div>
        <script type="text/javascript">

        var getBoxEdgesTable = [
            [0,8,9,17],    [1,9,10,18],   [2,10,11,19],
            [3,11,12,20],  [4,12,13,21],  [5,13,14,22],
            [6,14,15,23],  [7,15,16,24],  [17,25,26,34],
            [18,26,27,35], [19,27,28,36], [20,28,29,37],
            [21,29,30,38], [22,30,31,39], [23,31,32,40],
            [24,32,33,41], [35,42,43,48], [36,43,44,49],
            [39,45,46,50], [40,46,47,51], [48,52,53,58],
            [49,53,54,59], [50,55,56,60], [51,56,57,61],
            [58,62,63,68], [59,63,64,69], [60,65,66,70],
            [61,66,67,71]
        ];

        function isBoxTaken(state, box) {{
            var boxEdges = getBoxEdgesTable[box];
            for (edge in boxEdges) {{
                if (state[edge] == '0') {{
                    return false;
                }}
            }}
            return true;
        }}

        function stateStringToViz(state) {{
            vertical = '|';
            horizontal = '―';
            dot = '•';
            cross = '✕';
            empty = ' ';

            viz = "";
            viz += dot;
            ei = 0; // edge index
            si = 0; // square index

            // First 2 rows of squares:
            for(; ei<72; ei++) {{
                if ((ei >=0  && ei <=7)  ||
                    (ei >=17 && ei <=24) ||
                    (ei >=34 && ei <=41) ||
                    (ei >=48 && ei <=51) ||
                    (ei >=58 && ei <=61) ||
                    (ei >=68 && ei <=71)) {{

                    if (state[ei] == '0') {{
                        viz += empty;
                    }}
                    else {{
                        viz += horizontal;
                    }}
                    viz += dot;

                    if (ei==7 || ei==24) {{
                        viz += "\\n";
                    }}
                    else if (ei==41) {{
                        viz += "\\n  ";
                    }}
                    else if (ei==49 || ei==59 || ei==69) {{
                        viz += "   " + dot;
                    }}
                    else if (ei==51 || ei==61) {{
                        viz += "  \\n  ";
                    }}
                }}
                else {{
                    // Rows with vertical edges
                    // e.g. ei=8
                    
                    if(state[ei] == '0') {{
                        viz += empty;
                    }}
                    else {{
                        viz += vertical;
                    }}

                    if (ei==16 || ei==33) {{
                        viz += "\\n" + dot;
                    }}
                    else if (ei==44 || ei==54 || ei==64) {{
                        viz += "   ";
                    }}
                    else if (ei==47 || ei==57 || ei==67) {{
                        viz += "  \\n  " + dot;
                    }}
                    else {{
                        if (isBoxTaken(state, si)) {{
                            viz += cross;
                        }}
                        else {{
                            viz += empty;
                        }}
                        si++;
                    }}
                }}
            }}
            viz += "\\n";

            return viz;
        }}


        var nodes = new vis.DataSet({1});
        var edges = new vis.DataSet({2});

        var container = document.getElementById("network");
        var data = {{
            nodes: nodes,
            edges: edges
        }};
        var options = {{
            nodes: {{
                shape: 'dot',
                scaling: {{
                    min: 5,
                    max: 72
                }}
            }},
            layout: {{
                hierarchical: {{
                    direction: 'UD'
                }},
            }},
            physics: {{
                hierarchicalRepulsion: {{
                    nodeDistance: 300,
                    springLength: 300
                }}
            }}
        }};
        var network = new vis.Network(container, data, options);
        network.on("click", function (params) {{
            if(params.nodes.length == 1) {{
                nodeId = params.nodes[0];
                console.log("nodeId = " + nodeId);
                node = nodes.get(nodeId);

                viz = stateStringToViz(node.state);
                console.log(viz);
                console.log(JSON.stringify(node, null, '\\t'));

                tmpNode = node; // don't include state to save space
                delete tmpNode.state;

                document.getElementById("stateViz").innerHTML = viz + "\\n" + JSON.stringify(tmpNode, null, '  ');
            }}
        }})
        </script>
    </body>
</html>""".format(tree.getTitle(), nodes, edges)

if __name__ == '__main__':
    os.system("xdg-open http://localhost:5000/")
    app.debug = True
    app.run()
