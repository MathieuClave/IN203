# Prerequis

    sudo apt install g++-10 
    sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev

Et si besoin modifier Makefile et Make_linux.inc pour utiliser g++-10. 



lscpu : 12 CPUs



1) Mesure du temps 
    
    Temps moyen de calcul à chaque étape : 0.017 secondes
    Temps moyen d'affichage à chaque étape : 0.045 secondes, soit ~3 fois plus.
    (moyenne "à l'oeil", pas de vrai calcul statistique)

    Temps total pour 1000 jours : 62.2963 secondes


2) Parallélisation affichage contre simulation

    J'ai disséqué la grille pour envoyer chaque int un par un à chaque étape.

    Avec les envois et réceptions non comptés dans le temps :
    Temps moyen de calcul à chaque étape : 0.017 secondes 
    Temps moyen d'affichage à chaque étape : 0.045 secondes.
    (moyenne "à l'oeil", pas de vrai calcul statistique)
        On ne voit pas beaucoup de changement.

    Avec les envois et réceptions comptés dans le temps :
    Temps moyen de calcul à chaque étape : 0.08 secondes 
    Temps moyen d'affichage à chaque étape : 0.085 secondes.
    (moyenne "à l'oeil", pas de vrai calcul statistique)
        On voit ici un énorme ralentissement, il faut faire moins d'envois de données.

    Je regroupe les envois pour faire moins de discussion entre les processus :

    Avec les envois et réceptions comptés dans le temps :
    Temps moyen de calcul à chaque étape : 0.050 secondes 
    Temps moyen d'affichage à chaque étape : 0.047 secondes.
    (moyenne "à l'oeil", pas de vrai calcul statistique)
        Pas mal. On a presque rattrappé la vitesse initiale.

    
3) Parallélisation affichage asynchrone contre simulation

    Le Temps par étape n'a plus de sens.

    Temps total pour 1000 jours : 18.6 secondes, Speedup = 3.3


4) Parallélisation OpenMP

    Temps total pour 1000 jours : 10.1 secondes, Speedup = 6.2
    Je n'ai pas réussi à conserver la graine.
        

5) Parallélisation MPI de la simulation

    Paralléliser les mises-à-jour est une mauvaise idée en mémoire non-partagée.
    Il faut donc paralléliser le reste, puis gather les individus traités et les grilles modifiées.
    Pour les individus, il suffit de faire des vecteurs-copie plus courts, de traiter (impossible sans la grille), puis de réassembler. En revanche, pour la grille, s'il faut la réenvoyer à chaque fois avec la technique précédente(en O(population)), cela ne vaut pas le coup de paralléliser.
    Je n'ai pas de solution efficace à la Parallélisation MPI.
    Conclusion, je n'ai probablement pas compris la consigne.


5.1) Parallélisation finale  

    Je n'ai pas réussi MPI, et je n'ai pas accès à un réseau d'ordinateur.


5.2) Bilan

    L'exercice 4) donne un speedup de 6, ce qui est plutôt bon pour la configuration de mon ordinateur.
    Cette version combine des techniques MPI et OpenMP, ce qui a rendu leurs interactions plus claires à mes yeux.