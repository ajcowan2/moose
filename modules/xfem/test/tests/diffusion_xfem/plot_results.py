#!/usr/bin/env python

# Wen Jiang
# Date:

import csv
import matplotlib as mpl
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.ticker import LinearLocator, FormatStrFormatter
from matplotlib import cm
mpl.use('Agg')
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from collections import defaultdict
from os import system
import numpy as np
import scipy
from scipy.interpolate import griddata
from scipy.spatial import distance
from scipy import stats
from scipy.stats import t
from matplotlib import gridspec

from matplotlib.ticker import MultipleLocator

import pandas as pd

cjam= ['tab:blue','tab:orange','tab:green','tab:red','tab:purple','tab:brown','tab:olive']
msList= ['o','s','D','^','>','v','<','d','p','h','H','8','P','*','X','+','x']

tk_array= np.arange(300,3010,10)

fig = plt.figure(figsize=[6.5,5.5])
gs = gridspec.GridSpec(1,1)
ax = fig.add_subplot(gs[0])

data = pd.read_csv("./plot-data.csv")
x=data['x']
y=data['y']
ax.plot(x,y,linestyle = 'None',label='experiment',c='green',marker = 'o')

data = pd.read_csv("./plot-data-2.csv")
x=data['x']
y=data['y']
ax.plot(x,y,'-',label='Solution from [1]',c='blue')

data = pd.read_csv("./pencil_electrode_corrosion_test_out.csv")
x=np.sqrt(data['time'])
y=(0.0160-data['interface_location'])*10000
ax.plot(x,y,'-',label='MOOSE',c='red')

plt.xlabel('Time$^{0.5} [s^{0.5}]$')
plt.ylabel('Pit Depth d [$\mu$m]')
ax.set_ylim(bottom=0)
plt.legend(framealpha=1.0, loc="best", bbox_to_anchor=(1, 1))

ax.minorticks_on()
plt.savefig('./compare.pdf', bbox_inches='tight');
plt.close(fig)
