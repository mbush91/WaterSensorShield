#
# Example python script to generate a BOM from a KiCad generic netlist
#
# Example: Sorted and Grouped CSV BOM
#

"""
    @package
    Output: CSV (comma-separated)
    Grouped By: Value, Footprint
    Sorted By: Ref
    Fields: Reference designators,Qty,Value,Manufacturer,Manufacturer Part#,Footprint,Description,Vendor,Vendor Part Number,Price,Datasheet

    Command line:
    python "pathToFile/bom_csv_grouped_by_value_with_fp.py" "%I" "%O.csv"
"""

# Import the KiCad python helper module and the csv formatter
import sys
sys.path.append("/Applications/KiCad/KiCad.app/Contents/SharedSupport/plugins")
import kicad_netlist_reader
import kicad_utils
import csv

SEEED = True

# A helper function to convert a UTF8/Unicode/locale string read in netlist
# for python2 or python3
def fromNetlistText( aText ):
    if sys.platform.startswith('win32'):
        try:
            return aText.encode('utf-8').decode('cp1252')
        except UnicodeDecodeError:
            return aText
    else:
        return aText

sys.argv.append("/Users/mikebush/GIT/PHH/beacon-pcb/beacon-pcb/beacon-pcb.xml")
sys.argv.append("Users/mikebush/GIT/PHH/beacon-pcb/beacon-pcb/beacon-pcb.csv")

# Generate an instance of a generic netlist, and load the netlist tree from
# the command line option. If the file doesn't exist, execution will stop
net = kicad_netlist_reader.netlist(sys.argv[1])

# Open a file to write to, if the file cannot be opened output to stdout
# instead
try:
    f = kicad_utils.open_file_write(sys.argv[2], 'w')
except IOError:
    e = "Can't open output file for writing: " + sys.argv[2]
    print(__file__, ":", e, sys.stderr)
    f = sys.stdout

# Create a new csv writer object to use as the output formatter
out = csv.writer(f, delimiter=',', quotechar='\"', quoting=csv.QUOTE_ALL)

# Output a set of rows for a header providing general information
out.writerow(['Source:', net.getSource()])
out.writerow(['Date:', net.getDate()])
out.writerow(['Tool:', net.getTool()])
out.writerow( ['Generator:', sys.argv[0]] )
out.writerow(['Component Count:', len(net.components)])

# DIGIKEY / MOUSER / SIERRA CIRCUITS
if not SEEED:
    out.writerow(['Reference designators', 'Qty', 'Value','Manufacturer', 'Manufacturer Part #','Footprint', 'Description', 'Vendor','Vendor Part Number', 'Price', 'Datasheet'])

# SEEED STUDIO
elif SEEED:
    out.writerow(['Designator', 'Manufacturer Part Number or Seeed SKU', 'Qty', 'Link'])

else:
    sys.exit(1) # exit if neither SIERRA nor SEEED

# Get all of the components in groups of matching parts + values
# (see ky_generic_netlist_reader.py)
grouped = net.groupComponents()

# Output all of the component information
for group in grouped:
    refs = ""

    # Add the reference of every component in the group and keep a reference
    # to the component so that the other data can be filled in once per group
    refs = []
    for component in group:
        refs.append( fromNetlistText( component.getRef() ) )
        c = component

    refs = ", ".join(refs)

    if not SEEED:
        # Fill in the component groups common data
        out.writerow([
            refs,
            len(group),
            fromNetlistText( c.getValue() ),
            fromNetlistText( c.getField("Manufacturer") ),
            fromNetlistText( c.getField("MPN") ),
            fromNetlistText( c.getFootprint() ),
            fromNetlistText( c.getDescription() ),
            fromNetlistText( "Digi-Key" ),
            fromNetlistText( c.getField("DKPN") ),
            fromNetlistText( c.getField("Price") ),
            fromNetlistText( c.getDatasheet() )
        ])

    elif SEEED:

        pn = c.getField("Seeed Alt.") if c.getField("Seeed Alt.") != '' else c.getField("MPN")

        out.writerow([
            refs,
            pn,
            len(group),
            fromNetlistText( c.getField("DK_Detail_Page") )
        ])
