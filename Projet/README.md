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
        