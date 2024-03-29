#                      DeClone                        


DeClone is a software for the prediction of ancestral adjacencies in reconciled
 gene trees.  
 
It is described in  
Cedric Chauve, Yann Ponty & João Paulo Pereira Zanetti; "Evolution of genes neighborhood within reconciled phylogenies: an ensemble approach."
BMC Bioinformatics 16:S6 (2015). https://doi.org/10.1186/1471-2105-16-S19-S6  

## 1. Installation/requirements
 DeClone requires the following pieces of software to be installed prior to 
 its compilation:
 
  a. Python 2.7 (may function with any version <3 but not tested);
  
  b. GNU gcc/g++ 4.8 and more recent
  
  c. GNU make

Once these requirements are met, DeClone can be compiled from its sources by
running make in the installation directory.

  Rem.: If your gcc version is prior to 4.8 and does not offer extensive 
  support for C++ 11, you may still compile DeClone with a restricted set of 
  features (i.e. no polytope propagation functionalities, used in the 
  parametric analysis) by instead running 
  
 ```
 make DeClone
 ```

## 2. Running DeClone

DeClone typically takes two reconciled gene trees, and an extant adjacency 
list, as input. It offers a variety of output types, as described below.

### 2.1 Usage and options
 ```
 Usage: DeClone [-t1|--tree1] v1 [-t2|--tree2] v2 [-a|--adjacencies] adj [opts]

 Where
    v1  - (Path to) Gene Tree 1 (Newick format)
    v2  - (Path to) Gene Tree 2 (Newick format)
    adj - Path to a list of adjacent extant genes
    
    Modes (default: -p):
      -b,--backtrack k   - Stochastic sampling of k adjacency trees
      -c,--count-coopts  - Count the number of co-optimal adjacency trees
      -h,--help          - Displays help and exits
      -i,--in-out        - Inside-outside mode
      -l,--adjpolytope   - Runs polytope propagation with gain cost, break cost
                           and 2 genes as parameters
                           Requires file with pairs of genes specified by node 
                           id in Newick file
      -n,--count         - Count the number of valid adjacency trees
      -p,--parsimony     - Maximum parsimony mode, returns the minimum cost for 
                           an adjacency forest (default)
      -s,--show-coopts   - Show all co-optimal adjacency trees
      -x,--all           - Exhaustive enumeration of adjacency trees
      -y,--polytope      - Runs polytope propagation with gain cost and break 
                           cost as parameters
      -z,--part-fun      - Computes partition function for instance

    Parameters:
      -d,--draw f        - Draws output to file f (mode-dependent)
      -kT val            - Sets Boltzmann 'constant' (i.e. temperature) to a 
                           given value (def.=1.0)
      -m,--matrix        - Outputs a matrix for the adjacency tree (only for -s 
                           and -b modes)
      -r,--rescale val   - Sets rescaling factor (def.=1.0)
      -sc,--score g b    - Sets costs for adjacency gains (g) and breaks (b) 
                           (def.=(1.0,1.0))
      -v,--verbose       - Verbose mode, provides more (possibly unnecessary) 
                           information
```

### 2.2 Input formats

#### 2.2.a (Extended) Newick gene trees

The input gene trees must be binary trees specified in Extended Newick 
(NHX) format. The branch lengths may default to 1.0. Each leaf node must
have one of the following as its node name.
  - If it is an extant gene, UNIQUE_NAME|SPECIES_NAME. 
  - If it is a gene loss, LOSS|SPECIES_ID. 

Every node must have the following information encoded in the NHX list. 
  - An event Ev (equals one of GDup, Spec, GLos).
  - An integer species id.

 Example: 
```
        ((((ENSP00000386947|Homo_sapiens:1.0[&&NHX:D=?:Ev=Extant:S=0:ND=0],ENSPTRP00000022397|Pan_troglodytes:1.0[&&NHX:D=?:Ev=Extant:S=1:ND=1]):1.0[&&NHX:D=?:Ev=Spec:S=2:ND=2],Loss|Gorilla_gorilla:1.0[&&NHX:D=?:Ev=GLos:S=3:ND=3]):1.0[&&NHX:D=?:Ev=Spec:S=4:ND=4],ENSPPYP00000014898|Pongo_pygmaeus:1.0[&&NHX:D=?:Ev=Extant:S=5:ND=5]):1.0[&&NHX:D=?:Ev=Spec:S=6:ND=6],ENSMMUP00000014141|Macaca_mulatta:1.0[&&NHX:D=?:Ev=Extant:S=7:ND=7])[&&NHX:D=?:Ev=Spec:S=8:ND=8];
```
Note 1: The tree file consists of a single line.
Note 2: The numeric gene IDs follow a depth-first pattern, accompanied by some offset.


#### 2.2.b Extant adjacency list 

The extant adjacency list must contain pairs of extant genes, referred 
to by their unique name at the leaves of the tree. The genes in each 
pair must be separated by a single space. The pairs themselves must be  
separated by a newline.

 Example:
 ```
            ENSMODP00000016865 ENSMODP00000008552
            ENSMODP00000008552 ENSMODP00000008623
            ENSMODP00000008623 ENSMODP00000015313
                    .................
                    .................
 ``` 

#### 2.2.c Ancestral adjacency list (Polytope propagation, i.e. '-l' option)

 The ancestral adjacency list consists of pairs of ancestral gene ids on 
 each line. The ids of genes from the first gene tree are listed in the 
 first column, and those from the second gene tree are list in the 
 second column. In a line, the ids are separated by a space. 

 It is important to note that the order in which the gene trees and 
 ancestral adjacencies are ordered matters. This is because there is no 
 unique id for an ancestral gene, and two such genes on different gene 
 trees may have the same id as assigned by the DFS.

 Example:
```
 100 0
 101 2
 103 4
```

### 2.3 Output types

#### 2.3.a Adjacency forests
```
   Modes: -k, -s, -x
```
Adjacency forests are specified as a set of NHX trees, with each 
internal node representing an ancestral adjacency, the event at that 
node, the species that the adjacency would be in, and a node id 
assigned through DFS (post-order).

Example:
```
((ENSPPYP00000014896-ENSPPYP00000014898|Pongo_pygmaeus[&&NHX:D=?:Ev=Extant:S=5:ND=2],((ENSPTRP00000044415-ENSPTRP00000022397|Pan_troglodytes[&&NHX:D=?:Ev=Extant:S=1:ND=5],Loss|100-0[&&NHX:D=?:Ev=GLos:S=0:ND=6])[&&NHX:D=F:Ev=Spec:S=2:ND=4],ADJLoss|102-3[&&NHX:D=?:Ev=ALos:S=3:ND=7])[&&NHX:D=F:Ev=Spec:S=4:ND=3])[&&NHX:D=F:Ev=Spec:S=6:ND=1],ENSMMUP00000024875-ENSMMUP00000014141|Macaca_mulatta[&&NHX:D=?:Ev=Extant:S=7:ND=8])[&&NHX:D=F:Ev=Spec:S=8:ND=0];
```

#### 2.3.b Matrices
```
   Modes: -k,-s,-x (with -m option), -i by default
```
The matrix output first lists the set of extant leaf adjacencies, in 
the same format as the input leaf adjacencies. This is followed by a 
matrix, with the rows annotated by DFS ids of nodes from the first 
gene tree, and the columns annotated by the DFS ids of nodes from the 
second gene tree. 

The entry (i,j), i and j being annotations (not row and column 
numbers), in the matrix can carry the following information.
  - The entry is 1 if the adjacency is present, and 0 otherwise.
  - For the -i option, the entry represents the probability that the 
  adjacency is present in an adjacency forest sampled randomly from a 
  Boltzmann distribution.

Example:
``` 
            > ENSMMUP00000024875 ENSMMUP00000014141 105 7
            > ENSPTRP00000044415 ENSPTRP00000022397 99 1
            > ENSPPYP00000014896 ENSPPYP00000014898 98 5
            
                7 5 3 1 0 2 4 6 8 
            105 1 0 0 0 0 0 0 0 0 
            102 0 0 0.6084277365 0 0 0 0 0 0 
            100 0 0 0 0 0.6292753968 0 0 0 0 
            99  0 0 0 1 0 0 0 0 0 
            101 0 0 0 0 0 0.7797459363 0 0 0 
            103 0 0 0 0 0 0 0.7346325706 0 0 
            98  0 1 0 0 0 0 0 0 0 
            104 0 0 0 0 0 0 0 0.8364157524 0 
            106 0 0 0 0 0 0 0 0 0.8185046745 
```


#### 2.3.c Polytopes
```
   Modes: -y, -l
```
There are two types of polytopes that can be calculated. The option -y 
computes a 2D polytope and the normals of its facets.

Example:
```
            Polygon: {{0,0},{0,2},{2,3},{4,3},{5,0},{5,2}}
            Normals (+Signatures): 
            {
              {-1,-0} -> {{5,0},{5,2}},
              {-0.7071067812,-0.7071067812} -> {{5,2},{4,3}},
              {0.4472135955,-0.894427191} -> {{2,3},{0,2}},
              {0,1} -> {{0,0},{5,0}},
              {1,0} -> {{0,2},{0,0}},
              {-0,-1} -> {{4,3},{2,3}}
            }
```
Here, the first line lists the vertices of the polytope. The subsequent 
lines list the normals, with each normal assigned to the set of vertices
which define the facet the normal belongs to.

The option -l calculates, for a given set of possible ancestral 
adjacencies, a set of 3D polytopes, one assigned to each adjacency. The 
format for describing the polytope remains the same.

 Example:
 ```
            Adjacency: 104,6
            Polygon: {{0,0,1},{0,2,1},{2,0,1},{2,1,0},{2,3,0},{3,0,0},{4,2,1},{4,3,0},{5,0,0},{5,1,1},{5,2,0}}
            Normals (+Signatures): 
            {
              {0,0,1} -> {{5,2,0},{3,0,0},{2,3,0}},
              {0,0,1} -> {{5,0,0},{3,0,0},{5,2,0}},
              {0,0,1} -> {{5,2,0},{2,3,0},{4,3,0}},
              {0.4472135955,-0,0.894427191} -> {{0,2,1},{2,3,0},{0,0,1}},
              {-0.5773502692,-0.5773502692,-0.5773502692} -> {{4,2,1},{5,1,1},{4,3,0}},
              {-0,-0,-1} -> {{4,2,1},{0,0,1},{5,1,1}},
              {0,-0.7071067812,-0.7071067812} -> {{4,2,1},{2,3,0},{0,2,1}},
              {-0,-0,-1} -> {{4,2,1},{0,2,1},{0,0,1}},
              {0,0,1} -> {{3,0,0},{2,1,0},{2,3,0}},
              {-0,-0,-1} -> {{5,1,1},{0,0,1},{2,0,1}},
              {-1,-0,-0} -> {{5,1,1},{5,0,0},{5,2,0}},
              {0,1,0} -> {{5,0,0},{0,0,1},{3,0,0}},
              {-0,-0.7071067812,-0.7071067812} -> {{4,3,0},{2,3,0},{4,2,1}},
              {-0.5773502692,-0.5773502692,-0.5773502692} -> {{4,3,0},{5,1,1},{5,2,0}},
              {-0.2294157339,0.6882472016,-0.6882472016} -> {{5,1,1},{2,0,1},{5,0,0}},
              {0,1,0} -> {{2,0,1},{0,0,1},{5,0,0}},
              {0.4472135955,0,0.894427191} -> {{2,1,0},{0,0,1},{2,3,0}},
              {0.3015113446,0.3015113446,0.9045340337} -> {{3,0,0},{0,0,1},{2,1,0}}
            }
            ----------------
            Adjacency: 106,8
            ...
            ...
            ...
```

### 2.4 Advanced options
    ... add discussion about Rescaling and kT ...

## 3. References

   Bérard, Sèverine, Coralie Gallien, Bastien Boussau, Gergely J. Szöllősi, 
   Vincent Daubin, and Eric Tannier. "Evolution of gene neighborhoods within 
   reconciled phylogenies." Bioinformatics 28, no. 18 (2012): i382-i388.


   Barber, C. Bradford, David P. Dobkin, and Hannu Huhdanpaa. "The quickhull 
   algorithm for convex hulls." ACM Transactions on Mathematical Software 
   (TOMS) 22, no. 4 (1996): 469-483.

   Rajaraman, Ashok, Chauve, Cedric and Ponty, Yann. "Assessing the Robustness 
   of Parsimonious Predictions for Gene Neighborhoods from Reconciled 
   Phylogenies." 
   Bioinformatics Research and Applications (2015): 260-271.

   Zanetti, João Paulo Pereira, Yann Ponty, and Cedric Chauve. "Evolution of 
   genes neighborhood within reconciled phylogenies: an ensemble approach." In 
   BSB-Brazilian Symposium on Bioinformatics-2014. 2014.
