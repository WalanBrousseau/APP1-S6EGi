# GPL3, Copyright (c) Max Hofheinz, UdeS, 2021

import fiddle
import numpy
from subprocess import Popen, PIPE
import mmap
import time

FNAME        = "GIF642-labo-shm1"
FNAME2       = "GIF642-labo-shm2"
MATRIX_SIZE = 100

def subp():
    subproc = Popen(["./ftdt_yee", FNAME,FNAME2], stdin=PIPE, stdout=PIPE)
    return subproc

def signal_and_wait(subproc):
    subproc.stdin.write("START\n".encode())
    subproc.stdin.flush()                   # Nécessaire pour vider le tampon de sortie
    res = subproc.stdout.readline()
    #print(res)


# Lancement de l'exécutable associé
# NOTE: suppose que l'exécutable est dans le même dossier que celui en cours (normalement build/)
subproc = subp()
# Envoi d'une ligne sur l'entrée du sous-processus et attend un retour pour signaler que
# nous sommes prêts à passer à la prochaine étape. 
signal_and_wait(subproc)
shm_f = open(FNAME, "r+b")
shm_f2 = open(FNAME2, "r+b") 
shm_mm = mmap.mmap(shm_f.fileno(), 0)
shm_mm2 = mmap.mmap(shm_f2.fileno(), 0)
shared_matrix = numpy.ndarray(shape=(MATRIX_SIZE, MATRIX_SIZE, MATRIX_SIZE, 3), dtype=numpy.float64, buffer=shm_mm)
shared_matrix2 = numpy.ndarray(shape=(MATRIX_SIZE, MATRIX_SIZE, MATRIX_SIZE, 3), dtype=numpy.float64, buffer=shm_mm2)
shared_matrix[:] = numpy.zeros(shape=shared_matrix.shape)
shared_matrix2[:] = numpy.zeros(shape=shared_matrix2.shape)

class WaveEquation:
    def __init__(self, s, courant_number, source):
        s = s + (3,)
        self.E = shared_matrix
        self.H = shared_matrix2
        self.courant_number = courant_number
        self.source = source
        self.index = 0


    def __call__(self, figure, field_component, slice, slice_index, initial=False):
        if field_component < 3:
            field = self.E
        else:
            field = self.H
            field_component = field_component % 3
        if slice == 0:
            field = field[slice_index, :, :, field_component]
        elif slice == 1:
            field = field[:, slice_index, :, field_component]
        elif slice == 2:
            field = field[:, :, slice_index, field_component]
        signal_and_wait(subproc)    

        if initial:
            axes = figure.add_subplot(111)
            self.image = axes.imshow(field, vmin=-1e-2, vmax=1e-2)
        else:
            self.image.set_data(field)
        self.index += 1


n = MATRIX_SIZE
r = 0.01
l = 30


def source(index):
    return ([n // 3], [n // 3], [n // 2],[0]), 0.1*numpy.sin(0.1 * index)

w = WaveEquation((n, n, n), 0.1, source)
fiddle.fiddle(w, [('field component',{'Ex':0,'Ey':1,'Ez':2, 'Hx':3,'Hy':4,'Hz':5}),('slice',{'XY':2,'YZ':0,'XZ':1}),('slice index',0,n-1,n//2,1)], update_interval=0.01)

print("PY:  Done")

subproc.kill()
shm_mm.close() 
shm_mm2.close() 
