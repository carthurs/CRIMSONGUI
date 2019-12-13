import re
import string

newdata = ""
for l in open("TKPCAF.vcxproj"):
    if (re.search("[^t][pcPC]:/", l)):
        l = re.sub('([^<])/', r'\1\\', l)
    newdata = newdata + l
    
open("TKPCAF.vcxproj", "wt").write(newdata)