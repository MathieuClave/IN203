# include <cstdlib>
# include <sstream>
# include <string>
# include <fstream>
# include <iostream>
# include <iomanip>
#include <math.h>
# include <mpi.h>

int highestPower (int number) {
	int compt = 0;
	int result = 0 ;
	while (number >= pow(2,compt)) {
		result = compt;
		compt ++;
	}
	return result;
}

int main( int nargs, char* argv[] )
{
	// On initialise le contexte MPI qui va s'occuper :
	//    1. Créer un communicateur global, COMM_WORLD qui permet de gérer
	//       et assurer la cohésion de l'ensemble des processus créés par MPI;
	//    2. d'attribuer à chaque processus un identifiant ( entier ) unique pour
	//       le communicateur COMM_WORLD
	//    3. etc...
	MPI_Init( &nargs, &argv );
	// Pour des raisons de portabilité qui débordent largement du cadre
	// de ce cours, on préfère toujours cloner le communicateur global
	// MPI_COMM_WORLD qui gère l'ensemble des processus lancés par MPI.
	MPI_Comm globComm;
	MPI_Comm_dup(MPI_COMM_WORLD, &globComm);
	// On interroge le communicateur global pour connaître le nombre de processus
	// qui ont été lancés par l'utilisateur :
	int nbp;
	MPI_Comm_size(globComm, &nbp);
	// On interroge le communicateur global pour connaître l'identifiant qui
	// m'a été attribué ( en tant que processus ). Cet identifiant est compris
	// entre 0 et nbp-1 ( nbp étant le nombre de processus qui ont été lancés par
	// l'utilisateur )
	int rank;
	MPI_Comm_rank(globComm, &rank);
	// Création d'un fichier pour ma propre sortie en écriture :
	std::stringstream fileName;
	fileName << "Output" << std::setfill('0') << std::setw(5) << rank << ".txt";
	std::ofstream output( fileName.str().c_str() );

	// Rajout du programme ici...
	int dimension = 4;

	if (rank==0) {
		int jeton = 3141592;
		for (int i = 0; i<dimension; i++){
			MPI_Send(&jeton, 1, MPI_INT, pow(2,i), 0, MPI_COMM_WORLD);
			std::cout << rank << " : Message sent : " << jeton <<" to : " << pow(2,i) << "\n";
		}
	}
	else {
		int jeton;
		MPI_Status status;
		int hP = highestPower(rank);
		MPI_Recv(&jeton, 1, MPI_INT, rank-hP, 0, MPI_COMM_WORLD ,&status);
		std::cout << rank << " : Message recieved : " << jeton <<"\n";
		for (int i = hP+1; i<dimension; i++){
			MPI_Send(&jeton, 1, MPI_INT, pow(2,i), 0, MPI_COMM_WORLD);
			std::cout << rank << " : Message sent : " << jeton <<" to : " << pow(2,i) << "\n";
		}
	}



	
	output.close();
	// A la fin du programme, on doit synchroniser une dernière fois tous les processus
	// afin qu'aucun processus ne se termine pendant que d'autres processus continue à
	// tourner. Si on oublie cet instruction, on aura une plantage assuré des processus
	// qui ne seront pas encore terminés.
	MPI_Finalize();
	return EXIT_SUCCESS;
}
