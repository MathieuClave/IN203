#include <cstdlib>
#include <random>
#include <iostream>
#include <fstream>
#include <chrono>
#include <mpi.h>
#include <omp.h>
#include "contexte.hpp"
#include "individu.hpp"
#include "graphisme/src/SDL2/sdl2.hpp"

void màjStatistique( épidémie::Grille& grille, std::vector<épidémie::Individu> const& individus )
{
#   pragma omp parallel for 
    for ( auto& statistique : grille.getStatistiques() )
    {
        statistique.nombre_contaminant_grippé_et_contaminé_par_agent = 0;
        statistique.nombre_contaminant_seulement_contaminé_par_agent = 0;
        statistique.nombre_contaminant_seulement_grippé              = 0;
    }
    auto [largeur,hauteur] = grille.dimension();
    auto& statistiques = grille.getStatistiques();

#   pragma omp parallel for 
    for ( auto const& personne : individus )
    {
        auto pos = personne.position();

        std::size_t index = pos.x + pos.y * largeur;
        if (personne.aGrippeContagieuse() )
        {
            if (personne.aAgentPathogèneContagieux())
            {
                statistiques[index].nombre_contaminant_grippé_et_contaminé_par_agent += 1;
            }
            else 
            {
                statistiques[index].nombre_contaminant_seulement_grippé += 1;
            }
        }
        else
        {
            if (personne.aAgentPathogèneContagieux())
            {
                statistiques[index].nombre_contaminant_seulement_contaminé_par_agent += 1;
            }
        }
    }
}

void afficheSimulation(sdl2::window& écran, épidémie::Grille const& grille, std::size_t jour)
{
    auto [largeur_écran,hauteur_écran] = écran.dimensions();
    auto [largeur_grille,hauteur_grille] = grille.dimension();
    auto const& statistiques = grille.getStatistiques();
    sdl2::font fonte_texte("./graphisme/src/data/Lato-Thin.ttf", 18);
    écran.cls({0x00,0x00,0x00});
    // Affichage de la grille :
    std::uint16_t stepX = largeur_écran/largeur_grille;
    unsigned short stepY = (hauteur_écran-50)/hauteur_grille;
    double factor = 255./15.;

    for ( unsigned short i = 0; i < largeur_grille; ++i )
    {
        for (unsigned short j = 0; j < hauteur_grille; ++j )
        {
            auto const& stat = statistiques[i+j*largeur_grille];
            int valueGrippe = stat.nombre_contaminant_grippé_et_contaminé_par_agent+stat.nombre_contaminant_seulement_grippé;
            int valueAgent  = stat.nombre_contaminant_grippé_et_contaminé_par_agent+stat.nombre_contaminant_seulement_contaminé_par_agent;
            std::uint16_t origx = i*stepX;
            std::uint16_t origy = j*stepY;
            std::uint8_t red = valueGrippe > 0 ? 127+std::uint8_t(std::min(128., 0.5*factor*valueGrippe)) : 0;
            std::uint8_t green = std::uint8_t(std::min(255., factor*valueAgent));
            std::uint8_t blue= std::uint8_t(std::min(255., factor*valueAgent ));
            écran << sdl2::rectangle({origx,origy}, {stepX,stepY}, {red, green,blue}, true);
        }
    }

    écran << sdl2::texte("Carte population grippée", fonte_texte, écran, {0xFF,0xFF,0xFF,0xFF}).at(largeur_écran/2, hauteur_écran-20);
    écran << sdl2::texte(std::string("Jour : ") + std::to_string(jour), fonte_texte, écran, {0xFF,0xFF,0xFF,0xFF}).at(0,hauteur_écran-20);
    écran << sdl2::flush;
}

void simulation(bool affiche)
{
    int nbp, rank;
    MPI_Comm globComm;
	MPI_Comm_dup(MPI_COMM_WORLD, &globComm);
	MPI_Comm_size(globComm, &nbp);
    MPI_Comm_rank(globComm, &rank);
    MPI_Status status;
    int tag = 101; //Osef de la valeur
    int signal; //Signifie : je suis en attente de la suite.

    if(rank == 0){ //Affichage

        std::chrono::time_point<std::chrono::system_clock> start, end, start_jours, end_jours;
        std::chrono::duration<double> temps_affichage;
        std::chrono::duration<double> temps_total;
        start_jours = std::chrono::system_clock::now();

        constexpr const unsigned int largeur_écran = 1280, hauteur_écran = 1024;
        sdl2::window écran("Simulation épidémie de grippe", {largeur_écran,hauteur_écran});

        épidémie::ContexteGlobal contexte;
        épidémie::Grille grille{contexte.taux_population};

        auto [largeur_grille,hauteur_grille] = grille.dimension();
        auto& statistiques = grille.getStatistiques();

        sdl2::event_queue queue;
        bool quitting = false;

        std::ofstream output("Courbe.dat");
        output << "# jours_écoulés \t nombreTotalContaminésGrippe \t nombreTotalContaminésAgentPathogène()" << std::endl;

        std::cout << "Début boucle épidémie" << std::endl << std::flush;

        unsigned int jours_écoulés;
        double temps_calcul;

        while (!quitting){

            auto events = queue.pull_events();
            for ( const auto& e : events)
            {
                if (e->kind_of_event() == sdl2::event::quit)
                    quitting = true;
            }

            //Réception des calculs : grille, jours_écoulés, temps_calcul
            start = std::chrono::system_clock::now(); 

            int taille = largeur_grille*hauteur_grille;
            std::vector<int> grippe_vector(taille), agent_vector(taille), les_deux_vector(taille);
            MPI_Send(&signal, 1, MPI_INT, 1, tag, globComm); //Préviens qu'il attends la suite

            MPI_Recv(grippe_vector.data(), taille, MPI_INT, 1, MPI_ANY_TAG, globComm, &status);
            MPI_Recv(agent_vector.data(), taille, MPI_INT, 1, MPI_ANY_TAG, globComm, &status); 
            MPI_Recv(les_deux_vector.data(), taille, MPI_INT, 1, MPI_ANY_TAG, globComm, &status);
            
            for ( unsigned short i = 0; i < largeur_grille; ++i ){
                for (unsigned short j = 0; j < hauteur_grille; ++j ){
                    auto& stat = statistiques[i+j*largeur_grille];
                    
                    stat.nombre_contaminant_seulement_grippé = grippe_vector[i+j*largeur_grille];
                    stat.nombre_contaminant_seulement_contaminé_par_agent = agent_vector[i+j*largeur_grille];
                    stat.nombre_contaminant_grippé_et_contaminé_par_agent = les_deux_vector[i+j*largeur_grille];
                }
            }
            
            MPI_Recv(&jours_écoulés, 1, MPI_UNSIGNED, 1, MPI_ANY_TAG, globComm, &status);
            MPI_Recv(&temps_calcul, 1, MPI_DOUBLE, 1, MPI_ANY_TAG, globComm, &status);

            //#############################################################################################################
            //##    Affichage des résultats pour le temps  actuel
            //#############################################################################################################
            
            if (affiche) afficheSimulation(écran, grille, jours_écoulés);
            
            /*std::cout << jours_écoulés << "\t" << grille.nombreTotalContaminésGrippe() << "\t"
                    << grille.nombreTotalContaminésAgentPathogène() << std::endl;*/

            output << jours_écoulés << "\t" << grille.nombreTotalContaminésGrippe() << "\t"
                << grille.nombreTotalContaminésAgentPathogène() << std::endl;
            

            end = std::chrono::system_clock::now();
            temps_affichage = end - start;
            //std::cout << jours_écoulés << " jours écoulés." << std::endl;
            //std::cout << "Calcul : " << temps_calcul << "      Affichage : " << temps_affichage.count() << std::endl;

            if(jours_écoulés > 1000){
                end_jours = std::chrono::system_clock::now();
                temps_total = end_jours - start_jours;
                std::cout << jours_écoulés << " jours écoulés, temps total : " << temps_total.count() << std::endl;
            }
            if(jours_écoulés > 1001) quitting =true;
            
        }

        output.close();
    }



    if (rank == 1){ //Calcul

        std::chrono::time_point<std::chrono::system_clock> start, end;
        std::chrono::duration<double> temps_calcul;

        unsigned int graine_aléatoire = 1;
        std::uniform_real_distribution<double> porteur_pathogène(0.,1.);

        épidémie::ContexteGlobal contexte;
        // contexte.déplacement_maximal = 1; <= Si on veut moins de brassage
        // contexte.taux_population = 400'000;
        //contexte.taux_population = 1'000;
        contexte.interactions.β = 60.;
        std::vector<épidémie::Individu> population;
        population.reserve(contexte.taux_population);
        épidémie::Grille grille{contexte.taux_population};

        auto [largeur_grille,hauteur_grille] = grille.dimension();
        // L'agent pathogène n'évolue pas et reste donc constant...
        épidémie::AgentPathogène agent(graine_aléatoire++);
        // Initialisation de la population initiale :

        for (std::size_t i = 0; i < contexte.taux_population; ++i )
        {
            std::default_random_engine motor(100*(i+1));
            population.emplace_back(graine_aléatoire++, contexte.espérance_de_vie, contexte.déplacement_maximal);
            population.back().setPosition(largeur_grille, hauteur_grille);
            if (porteur_pathogène(motor) < 0.2)
            {
                population.back().estContaminé(agent);   
            }
        }

        std::size_t jours_écoulés = 0;
        int         jour_apparition_grippe = 0;
        int         nombre_immunisés_grippe= (contexte.taux_population*23)/100;
        sdl2::event_queue queue;
        bool quitting = false;

        épidémie::Grippe grippe(0);

        while (!quitting){

            auto events = queue.pull_events();
            for ( const auto& e : events)
            {
                if (e->kind_of_event() == sdl2::event::quit)
                    quitting = true;
            }

            if (jours_écoulés%365 == 0)// Si le premier Octobre (début de l'année pour l'épidémie ;-) )
            {
                grippe = épidémie::Grippe(jours_écoulés/365);
                jour_apparition_grippe = grippe.dateCalculImportationGrippe();
                grippe.calculNouveauTauxTransmission();
                // 23% des gens sont immunisés. On prend les 23% premiers
            #   pragma omp parallel for 
                for ( int ipersonne = 0; ipersonne < nombre_immunisés_grippe; ++ipersonne)
                {
                    population[ipersonne].devientImmuniséGrippe();
                }
            #   pragma omp parallel for
                for ( int ipersonne = nombre_immunisés_grippe; ipersonne < int(contexte.taux_population); ++ipersonne )
                {
                    population[ipersonne].redevientSensibleGrippe();
                }
            }
            if (jours_écoulés%365 == std::size_t(jour_apparition_grippe))
            {
            #   pragma omp parallel for 
                for (int ipersonne = nombre_immunisés_grippe; ipersonne < nombre_immunisés_grippe + 25; ++ipersonne )
                {
                    population[ipersonne].estContaminé(grippe);
                }
            }
            
            start = std::chrono::system_clock::now();

            // Mise à jour des statistiques pour les cases de la grille :
            màjStatistique(grille, population);
            // On parcout la population pour voir qui est contaminé et qui ne l'est pas, d'abord pour la grippe puis pour l'agent pathogène
            std::size_t compteur_grippe = 0, compteur_agent = 0, mouru = 0;
            unsigned nouvelle_graine;
        #   pragma omp parallel for shared(nouvelle_graine) 
            for ( auto& personne : population )
            {
                if (personne.testContaminationGrippe(grille, contexte.interactions, grippe, agent))
                {
                    compteur_grippe ++;
                    personne.estContaminé(grippe);
                }
                if (personne.testContaminationAgent(grille, agent))
                {
                    compteur_agent ++;
                    personne.estContaminé(agent);
                }
                // On vérifie si il n'y a pas de personne qui veillissent de veillesse et on génère une nouvelle personne si c'est le cas.
                if (personne.doitMourir())
                {
                    mouru++;
                    nouvelle_graine = jours_écoulés + personne.position().x*personne.position().y;
                    personne = épidémie::Individu(nouvelle_graine, contexte.espérance_de_vie, contexte.déplacement_maximal);
                    personne.setPosition(largeur_grille, hauteur_grille);
                }
                personne.veillirDUnJour();
                personne.seDéplace(grille);
            }
            jours_écoulés += 1;

            



            //Envoi des infos calculées : grille, jours_écoulés, temps_calcul
            int flag = 0;
            MPI_Iprobe( 0, MPI_ANY_TAG, globComm, &flag, &status );
            
            if (flag == 1) {
                
                MPI_Recv(&signal, 1, MPI_INT, 0, MPI_ANY_TAG, globComm, &status); //Libère le processus 0

                auto [largeur_grille,hauteur_grille] = grille.dimension();
                auto const& statistiques = grille.getStatistiques();
                
                int taille = largeur_grille*hauteur_grille;
                std::vector<int> grippe_vector(taille), agent_vector(taille), les_deux_vector(taille);

            #   pragma omp parallel for 
                for ( unsigned short i = 0; i < largeur_grille; ++i ){
                    for (unsigned short j = 0; j < hauteur_grille; ++j ){
                        auto& stat = statistiques[i+j*largeur_grille];
                        
                        grippe_vector[i+j*largeur_grille] = stat.nombre_contaminant_seulement_grippé;
                        agent_vector[i+j*largeur_grille] = stat.nombre_contaminant_seulement_contaminé_par_agent;
                        les_deux_vector[i+j*largeur_grille] = stat.nombre_contaminant_grippé_et_contaminé_par_agent; 
                    }
                }
                MPI_Send(grippe_vector.data(), taille, MPI_INT, 0, tag, globComm);
                MPI_Send(agent_vector.data(), taille, MPI_INT, 0, tag, globComm); 
                MPI_Send(les_deux_vector.data(), taille, MPI_INT, 0, tag, globComm); 

                unsigned int j = jours_écoulés;
                MPI_Send(&j, 1, MPI_UNSIGNED, 0, tag, globComm);

                end = std::chrono::system_clock::now();
                temps_calcul = end - start;
                double t = temps_calcul.count();
                MPI_Send(&t, 1, MPI_DOUBLE, 0, tag, globComm);
            }
            if(jours_écoulés > 1020) quitting = true;
        }
    }
}

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    // parse command-line
    bool affiche = true;
    for (int i=0; i<argc; i++) {
      std::cout << i << " " << argv[i] << "\n";
      if (std::string("-nw") == argv[i]) affiche = false;
    }
  
    sdl2::init(argc, argv);
    {
        simulation(affiche);
    }
    sdl2::finalize();

    MPI_Finalize();
    return EXIT_SUCCESS;
}

