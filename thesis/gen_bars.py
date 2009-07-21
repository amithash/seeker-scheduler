#! /usr/bin/python

from pyx import *
from fill_patterns import *

import pyx_common
import os, sys, re, string, operator, getopt, types


def titleToString(str):
    return string.replace(str, '_', '-')


fills = [[color.grey(.6),
          diagonal_bars(direction=1,density = 10),
          hatch(density = 10),
          color.grey(.3),
          diagonal_bars(direction=0,density = 10),
          polkadot(),
          inversepolkadot(),
          horizontal_bars(density = 10),
          color.grey.black,
          ]]

symbolstyles = [graph.style.symbol.cross,
                graph.style.symbol.plus,
                graph.style.symbol.square,
                graph.style.symbol.triangle,
                graph.style.symbol.circle,
                graph.style.symbol.diamond]

linestyles = [style.linestyle.solid,
              style.linestyle.dashed,
              style.linestyle.dotted,
              style.linestyle.dashdotted]
 
# Generate a legend: Width = width of the colored box, Height = height of the legend
def gen_legend(names, xoff, yoff, indices, width, height, stack, palette, stackheight, legendtextsize,rammode):
    lc = canvas.canvas()
    size = stackheight
    if(stackheight == 1):
        size = len(names)

    singleheight = float(height) / size        
    for i in range(size-1,-1,-1):
        if rammode:
            r = path.line(xoff, yoff+float(width)/2, width+xoff, yoff+float(width)/2)
            lc.stroke(r,[linestyles[i]])
            size = unit.topt(attr.selectattr(0.2*unit.v_cm, 0, 0))
            # xt,yt = lc.vpos_pt(xpos+float(width)/2,ypos+float(width)/2)
            xt = unit.topt((xoff+float(width)/2)*unit.v_cm)
            yt = unit.topt((yoff+float(width)/2)*unit.v_cm)
            symbolstyles[i].select(0,0)(lc,xt,yt,size,[deco.stroked])
            lc.text(xoff + width + .1 , yoff + .18, titleToString(names[i]), [trafo.scale(legendtextsize)])
        else:
            r = path.rect(xoff, yoff, width, singleheight)
            local_styles = map(lambda list,indices=indices,i=i : list[indices[i]-2], palette)
            lc.fill(r,local_styles)
            lc.stroke(r,[color.grey.black])        
            lc.text(xoff + width + .1 , yoff + .1, titleToString(names[i]), [trafo.scale(legendtextsize)])

        yoff -= singleheight
    return lc

# Read the column names from the data file
def read_labels(file, delim):
    infile = open(file,'r')
    line = infile.readline()
    commentline = None

    # Find the last comment line before an uncommented line
    # This should have the column headers
    while (len(line) != 0):
        line = line.strip()

        if (line[0] in ["#","!","%"]):
            commentline = line
        else:
            break

        line = infile.readline()  

    # Drop the x-axis label
    return commentline[1:].split(delim)[1:]


class stackable_legend:

    def __init__(self,keys):
        self.keys = keys
        self.hpos = keys[0].hpos
        self.hdist = keys[0].hdist
        self.hinside = keys[0].hinside
        self.vpos = keys[0].vpos
        self.vdist = keys[0].vdist
        self.vinside = keys[0].vinside

        
    
    def paint(self,plotitems):
        c = canvas.canvas()
        d = plotitems[0].data
        n = d.getcolumnnames(d)        
        for i in range(0,len(n)):
            dd = graph.data.data(d)
            c.insert(self.keys[0].paint(graph.graph.plotitem(None,dd,plotitems[0].styles)))
        return c

        
ignores = {}

class specialbaraxis(graph.axis.bar):
    def __init__(self,painter=graph.axis.painter.bar(), dist=0.5, multisubaxis=None, title=None):
        graph.axis.bar.__init__(self,painter=painter,dist=dist,multisubaxis=multisubaxis,title=title)

    def setname(self,name,*subnames):
        # print name, subnames
        if ignores.has_key(name) and len(subnames):
            if len(subnames) > 1:
                raise "jcone says FIXME: I don't handle this case"
            # print ignores[name], subnames[0], subnames[0].__class__
            # print ignores[name].keys()
            if ignores[name].has_key(subnames[0]):
                # print "ignoring ",name,subnames[0]
                return
        graph.axis.bar.setname(self,name,*subnames)


# This doesn't work - it returns too large a number and its the same number regardless of width
##def max_label_width(labels):
##    maxwidth = 0
##    for label in labels:
##        tr = text.texrunner().text(0, 0, titleToString(label))
##        width = tr.corners[2][1] - tr.corners[0][1]
##        #print label + " : " + `width`
##        if(width > maxwidth):
##            maxwidth = width     
##    return maxwidth

# Create a bar graph
def create(benchfile,miny,maxy,ytickdist,ylabeldist,ysplit,stack,logscale,xrotate,dolegend,dolabel,xtickheight,
           setprec,exponent,ytitle,labels,stacklabels,subaxisnameangle,
           indices,legendxshift,legendyshift,legendheight,legendwidth,legenddist,
           use_color,outfile, delim, xtitle, width, height, legendposition, barrelative,
           stackrelative, textcolumn, errorbars, xscale,subnamesize,rowexpr,yline,legendtextsize,rammode):

    # Ensures that we use Type 1 fonts in graphs!
    text.set(mode="latex")
    text.preamble(r"\usepackage[]{times}\usepackage{mathptm}")

    # y-axis numbers effectively start at 1, since pyx adds a column to the front for itself at 0
    # and the x-axis data is implied to be column 1
    for i in range(len(indices)):
        indices[i] = int(indices[i]) + 1

    # Number of parts to each column - effectively the stack height
    stackheight = stack

    # Text column adds to the stack height
    stacksize = stackheight
    if textcolumn:
        stacksize += 1

    if errorbars:
        stacksize += 1
    
    # Number of columns per x-axis category
    indices_length = len(indices)
    numcolumns = indices_length / stacksize

    # MJB: Need to change this to work correctly with textcolumn
    if (indices_length % stackheight) != (textcolumn + errorbars):
        print "Warning: Extra " + `len(indices) / stackheight` + " indices given"

    # Number of x-axis categories - set as data is read in
    numcategories = 0

    print "Columns/Category: " + str(numcolumns)
    print "Stack/Column: " + str(stackheight)

    legendlabels = []
    colstart = 0
    d = []
    # For each category, read in columns
    for i in range(numcolumns):
        # For multi-stackbars, the labels actually refer to the elements in
        # each stack.
        if (stackheight > 1) and (numcolumns > 1) and (not textcolumn):            
            label = labels[0]
            legendlabels = labels
        else:
            if (stackheight > 1) and (numcolumns == 1):
                label = labels[0]
                legendlabels = labels
            else:
                label = labels[colstart/stackheight]
                legendlabels += [label]

        stacks = {}
        stacks['y'] = indices[colstart]
        for i in range(1, stackheight):
            stacks['stack' + str(i)] = indices[colstart + i]

        colstart += stackheight

        # See if we have a text column
        if textcolumn:
            stacks['text'] = indices[colstart]
            colstart += 1

        if errorbars:
            stacks['dy'] = indices[colstart]
            colstart += 1

        data = pyx_common.readdata(benchfile, delim, xname = 1,  title = label, **stacks)

        # Filter out the rows we don't want
        for key in data.columns.keys():
            newrow = filter(rowexpr, data.columns[key][0])
            data.columns[key]=(newrow, data.columns[key][1])
        
        numcategories = len(data.columns['xname'][0])
        d += [data]


    # Figure out which palette set to use
    palette = fills
    if(use_color):
        palette = []
        length = numcolumns
        if(stackheight > 1):
            length = stackheight
        
        for i in range(length):
            # Move the column number into the range [0,1) so that we can use getcolor
            palette += [color.palette.Rainbow.getcolor(float(i)/length)]
        palette = [palette] # Palette should be a list of lists
            
    # The outline of each bar, add deco.style.THick for thicker lines
    outline = [deco.stroked([color.grey.black])]

    # If no stack labels set stacklabels to None
    if (len(stacklabels) == 0):
        stacklabels = None

    barstyles = []
    barstyles += [graph.style.barpos(fromvalue = barrelative, subnames = stacklabels)]
    
    # Stacked bar graphs are accomplished via styles
    # The first style automatically affects the bottom "y" bar
    if(stackheight > 1):
        local_styles = map(lambda list : list[0], palette)
        if rammode:
            barstyles += [graph.style.line(lineattrs=[linestyles[0]]),
                          graph.style.symbol(symbol=symbolstyles[0])]
        else:
            barstyles += [graph.style.bar(outline + local_styles)]            
    else:
        if(logscale):
            bar_type = graph.style.bar
        else:
            bar_type = pyx_common.broken_bar
        
        # attr.changelist doesn't work with one bar, so just put in the first element of each list in palette
        if(numcolumns > 1):
            local_styles = map(lambda list : attr.changelist(list), palette)
            barstyles += [bar_type(outline + local_styles)]
        else:
            local_styles = map(lambda list : list[0], palette)
            barstyles += [bar_type(outline + local_styles)]

    # Put text labels on the column
    if textcolumn:
        barstyles += [graph.style.text("text")]

    # If we're stacking bars, use stackedbarpos to set the position - then apply the styles 
    for i in range(1,stackheight):
        local_styles = map(lambda list,i=i : list[i], palette)
        barstyles += [graph.style.stackedbarpos("stack%s" % i)]    
        if rammode:
            barstyles += [graph.style.line(lineattrs=[linestyles[i]]),
                          graph.style.symbol(symbol=symbolstyles[i])]
        else:
            barstyles += [graph.style.bar(outline + local_styles)]

        
        #### FIXME: this is broken
        # Indicate if stack offset are absolute or relative
        #if (stacklabels != None) and not stackrelative:
        #    barstyles += [graph.style.barpos(fromvalue = barrelative)]

    if errorbars is not None:
        barstyles += [graph.style.errorbar()]

    labelattrs = []



    # Create the yaxis
    ypainter=graph.axis.painter.regular(titleattrs=[trafo.scale(1.0,1.0),text.halign.center])
    ytexter=graph.axis.texter.mixed(decimal=graph.axis.texter.decimal(equalprecision=1,labelattrs=[text.halign.right,text.mathmode]),
                                    exponential=graph.axis.texter.exponential(mantissatexter=graph.axis.texter.decimal(equalprecision=1, labelattrs=[text.halign.right,text.mathmode])))


    if (logscale):
        yparter = graph.axis.parter.autologarithmic()
        if (ytickdist):
            yparter = graph.axis.parter.logarithmic(ytickdist,ylabeldist)

        yaxis = graph.axis.logarithmic(painter = ypainter,title=ytitle, min=miny,max=maxy, parter=yparter, texter=ytexter)
    else:
        yparter = graph.axis.parter.autolinear()
        if (ytickdist):
            yparter = graph.axis.parter.linear(ytickdist,ylabeldist)

        if ysplit is None:
            yaxis = graph.axis.linear(painter = ypainter, title=ytitle, min=miny,max=maxy, parter=yparter, texter=ytexter)
        else:
            yaxes = []
            for axisrange in ysplit:
                axis = graph.axis.linear(painter = ypainter, min=axisrange[0],max=axisrange[1], parter=yparter, texter=ytexter)
                yaxes.append(axis)

            splitlist = [None for i in xrange(len(ysplit)-1)]
            yaxis = graph.axis.split(subaxes=yaxes, splitlist = splitlist, title=ytitle, relsizesplitdist=4)

    # Label Attributes for X axis labels
    
    xalign = text.halign.center
    if xrotate > 0:
        xalign = text.halign.right
        
    nameattrs = [trafo.rotate(xrotate),trafo.scale(xscale), xalign]

    if (not dolabel):
        nameattrs = None

    # Create the xaxis
    xpainter = graph.axis.painter.bar(nameattrs = nameattrs, innerticklength=xtickheight)
    if (numcolumns > 1):
        # When subnames exist, we want them to be small,
        # scale shoud be realtive to stack size and number of columns, but use .4 for now
        subpainter = graph.axis.painter.bar(nameattrs = [trafo.rotate(subaxisnameangle), trafo.scale(subnamesize), text.halign.center], namedist = 0.1)
        subaxis = graph.axis.bar(painter = subpainter, dist = 0)
        # graph.axis.bar(painter = subpainter, dist = 0)

        if(stacklabels != None):
#            xaxis = graph.axis.bar(painter = xpainter, multisubaxis=subaxis, dist = 1.5, title=xtitle)
            xaxis = specialbaraxis(painter = xpainter, multisubaxis=subaxis, dist = 1.5, title=xtitle)            
        else:
            xaxis = graph.axis.bar(painter = xpainter, subaxis=subaxis, dist = 1.5, title=xtitle)
    else:
        xaxis = graph.axis.bar(painter = xpainter, dist = 1.5, title=xtitle)

    # Create the legend using pyx methods if no stacking, otherwise we generate it ourselves
    legend = None
    generatedLegend = None
    if dolegend:
        # With a text column, pyx throws an error for the graph key
        if (stackheight == 1) and not textcolumn:
            legend = graph.key.key(vdist = legendyshift, hdist = legendxshift, dist=legenddist*unit.v_cm, **legendposition)
        else:
            # Some stuff to deal with generated legends, doesn't work well yet
            yoff = legendyshift
            if legendposition["pos"][0] == "t":
                yoff += height
            elif legendposition["pos"][0] == "m":
                yoff += float(height)/2
                
            if not legendposition["vinside"]:
                yoff += legendheight

            xoff = legendxshift
            if legendposition["pos"][1] == "r":
                xoff += width
            elif legendposition["pos"][1] == "c":
                xoff += float(width)/2

            # Multiply by 4 since we don't know how long a textrunner is
            if legendposition["hinside"]:
                xoff -= legendwidth

            generatedLegend = gen_legend(legendlabels, xoff, yoff, indices, legendwidth, legendheight, stack, palette, stackheight, legendtextsize,rammode)

    # Actually draw the graph
    gc = canvas.canvas()
    g = graph.graphxy(ypos=0, width=width, height=height, key = legend, x=xaxis, y=yaxis)
    gc = gc.insert(g)

    gc.plot(d, barstyles)

    gc.finish()

    if (yline is not None):
        ylinepos = yaxis.convert(yline)*height
        g.stroke(path.line(0,ylinepos,width,ylinepos),[style.linestyle.dashed])

      
    fc = canvas.canvas()
    fc.insert(gc)
    if generatedLegend is not None:
        fc.insert(generatedLegend)

    #bbox = fc.bbox()
    #fc.draw(bbox.enlarged(0.1).path(), [deco.stroked()])

    fc.writetofile(outfile)

def usage(msg = "", exitval = -1):
    print msg
    print """
Usage:
    gen_bars.py [options] <datafile>
Options:                                                                =default=
    --stack # : The number of data items to stack into a single column  [1]
    --barrelative #: The y-axis value to max the bars relative to       [Automatic]
    --ystart # : Set the zero point for the y-axis                      [Automatic]
    --yend # : Set the top for the y-axis                               [Automatic]
    --ytitle string : Set the y-axis title                              [\"\"]
    --ysplit {(min,max)} : Set the ranges for the split y-axis          [No splitting]
    --output outfile : Set the output file                              [<datafile>.eps]
    --index {#} : A comma separated list of indices to plot(first col=1)[All Indices]
    --labels {string}: A comma separated list of labels                 [All Labels]
    --rows {string}: A python lambda expression picking the needed rows [Automatic]
    --stacklabels {string}: A comma separated list of subaxis labels    [\"?\"]
    --subaxisnameangle {#}: Angle in degrees to rotate subaxis names    [0]
    --logscale : Use a logarithmic scale for the y-axis                 [False]
    --nolegend : turn off the legend                                    [False]
    --nolabel : turn off x-axis labels                                  [False]
    --xrotate : set the x-axis label rotation angle                     [45]
    --setprec # : Number of significant digits for numerical labels     [None]
    --xtickheight # : Height of the ticks on the x-axis                 [0.2]
    --yticks {string} : Comma seperated list of y-axis tick spacing     [None]
    --ylabels {string} : Comma seperated list of y-axis label spacing   [None]
    --width # : Width of the graph                                      [8]
    --height # : Height of the graph                                    [4]
    --legendpos +/-[t,m,b]+/-[l,c,r] : Position of legend               [-t+r]
    --legendxshift # : Shift the legend in the x-direction              [.08]
    --legendyshift # : Shift the legend in the y-direction              [-.8]
    --legendwidth #  : The width of the legend                          [.45]
    --legendheight # : The height of the legend                         [1.6]
    --legenddist # : The distance between legend items                  [0.2]
    --legendtextsize # : Size of the legend labels                      [0.75]
    --color : generate graphs in color                                  [False]
    --delim <delim> : The delimiter for the datafile                    [\t]
    --relative : Stack values are relative offset                       [False]
    --textcolumn : Consider the last stack to be a text stack           [False]
    --subnamesize : Size of the subnames                                [0.4]
    --fillpatterns : Specify what should fill each bar                  [Automatic]
    --yline # : Draw an extra line horizontal line                      [None]
    --rammode : Instead of drawing bars, draw a symbol-line             [False]
    --ignores # : Ignore certain columns in a multistackbar graph       [None]
                  Format: (label:stacklabel(,stacklabel)* )*
    """
    sys.exit(exitval)

def main():
    global fills

    datafile = ""
    stack = 1
    miny = 0
    maxy = None
    ysplit = None
    ytitle = ""
    xtitle = None
    stacklabels = []
    labels = []
    indices = []
    logscale = False
    xrotate = 45
    dolegend = True
    dolabel = True
    setprec = None
    exponent = None
    outfile = None
    delim="\t"
    width = 8
    height = 4
    legendpos = "-t+r"
    barrelative = None
    subaxisnameangle = 0
    stackrelative = None
    textcolumn = 0
    errorbars = 0
    xscale = 1
    xtickheight = 0.2
    skipnext = False
    yticks = None
    ylabels = None
    legendxshift = width * .01
    legendyshift = - height * .2
    legendwidth = 0.45
    legendheight = 1.6
    legenddist = 0.2
    yline = None
    use_color = 0
    subnamesize = 0.4
    rowexpr = eval("lambda row : 1")
    legendtextsize = 0.75
    rammode=False
    

    try:
        opts, args = getopt.getopt(sys.argv[1:],"",["ystart=","yend=","ytitle=","ysplit=","xtitle=", "output=","setprec=",
                                                    "exponent=","index=","labels=","stacklabels=",
                                                    "subaxisnameangle=",
                                                    "stack=","logscale","xrotate=","nolegend","nolabel",
                                                    "yticks=","ylabels=","legendxshift=","legendyshift=",
                                                    "legendheight=","legendwidth=","legenddist=",
                                                    "width=","height=","color","delim=","legendpos=", "barrelative=",
                                                    "relative", "textcolumn","errorbars","xscale=","xtickheight=","subnamesize=",
                                                    "fillpatterns=","rows=","yline=","legendtextsize=","rammode","ignores="])
    except getopt.GetoptError, e:
        usage(str(e))

    for opt,arg in opts:
        if (opt == "--ystart"):
            miny = float(arg)
        elif (opt == "--yend"):
            maxy = float(arg)
        elif (opt == "--ytitle"):
            ytitle = arg
        elif (opt == "--ysplit"):
            ysplit = eval(arg)
        elif (opt == "--xtitle"):
            xtitle = arg
        elif (opt == "--output"):
            outfile = arg
        elif (opt == "--setprec"):
            setprec = int(arg)
        elif (opt == "--exponent"):
            exponent = int(arg)
        elif (opt == "--index"):
            indices=string.split(arg,',')
        elif (opt == "--labels"):
            labels=string.split(arg,',')
        elif (opt == "--stacklabels"):
            stacklabels=string.split(arg,',')
        elif (opt == "--subaxisnameangle"):
            subaxisnameangle=int(arg)
        elif (opt == "--stack"):
            stack = int(arg)
        elif (opt == "--logscale"):
            logscale = True
        elif (opt == "--xrotate"):
            xrotate=int(arg)
        elif (opt == "--nolegend"):
            dolegend = False
        elif (opt == "--nolabel"):
            dolabel = False
        elif (opt == "--yticks"):
            yticks=string.split(arg,',')
        elif (opt == "--ylabels"):
            ylabels=string.split(arg,',')
            if yticks == None:
                yticks = ylabels
        elif (opt == "--legendxshift"):
            legendxshift=float(arg)
        elif (opt == "--legendyshift"):
            legendyshift=float(arg)
        elif (opt == "--legendheight"):
            legendheight=float(arg)
        elif (opt == "--legendwidth"):
            legendwidth=float(arg)
        elif (opt == "--legenddist"):
            legenddist=float(arg)
        elif (opt == "--color"):
            use_color = 1
        elif (opt == "--delim"):
            delim = arg
        elif (opt == "--width"):
            width = int(arg)
        elif (opt == "--height"):
            height = int(arg)
        elif (opt == "--legendpos"):
            legendpos = arg
        elif (opt == "--barrelative"):
            barrelative = arg
        elif (opt == "--relative"):
            stackrelative = 1
        elif (opt == "--textcolumn"):
            textcolumn = 1
        elif (opt =="--errorbars"):
            errorbars = 1
        elif (opt == "--xscale"):
            xscale = float(arg)
        elif (opt == "--subnamesize"):
            subnamesize = float(arg)
        elif (opt == "--xtickheight"):
            xtickheight = float(arg)
        elif (opt == "--fillpatterns"):
            fills = eval(arg)
            if(type(fills[0]) != types.ListType):
                fills = [fills]
        elif (opt == "--rows"):
            rowexpr = eval(arg)
        elif (opt == "--yline"):
            yline = float(arg)
        elif (opt == "--legendtextsize"):
            legendtextsize = float(arg)
        elif (opt == "--rammode"):
            rammode = True
        elif (opt == "--ignores"):
            ignore1 = arg.split(' ')
            for i in ignore1:
                n,rest = i.split(':')
                k = rest.split(',')
                ignores[n] = {}
                for kk in k:
                    ignores[n][kk] = 1
                
        else:
            usage("Unknown option " + opt)

    # Datafile should be the only argument left
    if len(args) < 1 or not os.path.exists(args[0]):
        usage("Datafile '%s' does not exist!" % datafile)
        
    datafile=args[0]

    # If no labels of indices are specified
    # Try and read them out of the file
    if (labels == []):
        labels = read_labels(datafile, delim)
        if(indices == []):
            indices = range(1,len(labels)+1)

    if (indices == []):
        if (labels != []):
            if (stacklabels != []):
                indices = range(1,len(labels)*len(stacklabels)+1)
            else:
                indices = range(1,len(labels)+1)

    # Fixup miny for log y-axis
    if (logscale and (miny == 0)):
        miny = 1

    # Determine the legend position
    if(len(legendpos) != 4):
        usage(legend + " is illegal legend position specification")

    # Currently only affects pyx generated legends, not our generated legends
    # - => Inside, + => Outside
    legendposition = {}
    legendposition["vinside"] = (legendpos[0] == '-')
    legendposition["hinside"] = (legendpos[2] == '-')
    legendposition["pos"] = legendpos[1]+legendpos[3]
    #print legendposition

    # Determine the outfile
    if (outfile == None):
        outfile = string.replace(os.path.basename(datafile), '.', '_')+".eps"
        if os.path.exists("figures"):
            outfile = os.path.join("figures",outfile)

    create(datafile,miny,maxy,yticks,ylabels,ysplit,stack,logscale,xrotate,dolegend,dolabel,xtickheight,setprec,exponent,ytitle,labels,stacklabels,subaxisnameangle,indices,legendxshift,legendyshift,legendheight,legendwidth,legenddist,use_color,outfile, delim, xtitle, width, height, legendposition, barrelative, stackrelative, textcolumn, errorbars, xscale,subnamesize,rowexpr,yline, legendtextsize,rammode)
    print "Generating Graph: " + outfile
    
if __name__=="__main__":
    main()
