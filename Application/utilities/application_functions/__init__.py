import os
from glob import glob as G
import subprocess as S
from glob import glob as G
import pytraj as pt
import math
import os
proc = S.Popen("which packmol",shell=True,stdout=S.PIPE,stderr=S.PIPE)
out,err= proc.communicate()
tmp = out.decode("utf-8").strip()
PACKMOL_EXE = tmp
     
class SolventMolecule():
    def __init__(self,pdbfile,mol2file,frcmodfile=None,n_mols=100):
        self.pdbfile    = pdbfile
        self.mol2file   = mol2file
        self.frcmodfile = frcmodfile
        self.n_mols     = int(n_mols)
        tmp = pt.load(pdbfile)
        self.molar_mass = sum([float(x.mass) for x in tmp.top.atoms])
        self.resname    = [x for x in tmp.top.atoms][0].resname
        self.tot_mass   = self.molar_mass * n_mols

class SolventMixtureBox():
    def __init__(self):
        self.molecules   = []
        self.density     = 1.0
        self.total_vol   = 0
        self.edge_length = 0.0

    def AddSolvent(self, pdbfile, mol2file, frcmodfile = None, n_mols = 100):
        new_mol = SolventMolecule(pdbfile,mol2file,frcmodfile,n_mols)
        self.molecules.append(new_mol)
        tmp_vol = new_mol.tot_mass / self.density * 1.6605778811026237
        self.total_vol += tmp_vol
        self.edge_length = math.ceil(self.total_vol ** 0.33333)

    def RunPackMol(self):
        with open("pack.inp","w") as f:
            f.write("""tolerance 2.0
filetype pdb
output mixture.pdb
add_amber_ter
""")
            for i,mol in enumerate(self.molecules):
                f.write(f"structure {mol.pdbfile}\n")
                f.write(f"number {mol.n_mols}\n")
                f.write(f"inside cube 0. 0. 0. {self.edge_length:>.01f}\n")
                f.write("end structure\n")
        with open("run_packmol.sh","w") as f:
            f.write("#!/bin/bash\n")
            f.write("echo 'Running packmol...'\n")
            f.write(f"{PACKMOL_EXE.strip()} < pack.inp > /dev/null\n")
        S.call("sh run_packmol.sh",shell=True)
        if G("mixture.pdb"):
            S.call("rm run_packmol.sh pack.inp",shell=True)
    def GenerateAmberLibrary(self,box_name="MIXTUREBOX",lib_name="mixturebox.lib",base_ffs=["leaprc.protein.ff14SB"]):
        with open("leap_library.in","w") as f:
            for ff in base_ffs:
                f.write(f"source {ff}\n")
            for i,mol in enumerate(self.molecules):
                f.write(f"{mol.resname} = loadmol2 {mol.mol2file}\n")
                if mol.frcmodfile:
                    f.write(f"loadamberparams {mol.frcmodfile}\n")
            f.write(f"{box_name} = loadpdb mixture.pdb\n")
            f.write(f"set {box_name} box "+"{"+f"{self.edge_length},{self.edge_length},{self.edge_length}"+"}\n")
            f.write(f"saveoff {box_name} {lib_name}\n")
            f.write("quit\n")
        S.call("tleap -f leap_library.in > /dev/null",shell=True)
        if not G(lib_name):
            print("Unable to generate library file.")
            return None
        S.call("rm leap.log leap_library.in mixture.pdb",shell=True)
        with open("solvent_tleap.in","w") as f:
            for ff in base_ffs:
                f.write(f"source {ff}\n")
            f.write("\n")
            f.write("## load solute molecule (protein, etc.)\n")
            f.write("# include mol2 and frcmod files.\n")
            f.write("# LIG = loadmol2 LIG.mol2\n")
            f.write("# loadamberparams LIG.frcmod\n")
            f.write("# mol = loadpdb protein.pdb\n")
            f.write("# check mol\n")
            f.write("\n")
            f.write("## add counterions where necessary.\n")
            f.write("# addions mol K+ 0\n")
            f.write("# addions mol Cl- 0\n")
            f.write("\n")
            f.write("## load custom solvent box library\n")
            f.write(f"loadoff {lib_name}\n\n")
            f.write("## solvate system with custom boxname (example to 20 Angstroms)\n")
            f.write(f"solvatebox mol {box_name} 20\n")
            f.write("\n")
            f.write("## save MD inputs\n")
            f.write("saveamberparm mol solvated.prmtop solvated.rst7\n")
            f.write("run\nquit\n")
        print("#"*60)
        print(open("solvent_tleap.in").read())
        print("#"*60)
        return None
