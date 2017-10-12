#include "evolution_methods.hpp"

//GLOBAL VARIABLES

//RUNTIME CONFIGURATIONS
int Target_Fitness=1;
int Colour_Space=7; 
int GENERATION_LIMIT=20000;
int Num_Runs=1000;
int Num_Genomes=10;
int Num_Tiles=2;
bool Regulated=false;
int Shape_Matching_Fitness=0;
int Fitness_Mode=0;
int Fitness_Oscillation_Rate=500;
int burn_in_period=100;//100000;
int mutation_cofactor=4;
int active_targets=3;
int initial_condition=0;
int RUN=0;

std::random_device rd;
xorshift RNG_Engine(rd());
const double MINIMUM_FITNESS_THRESHOLD=0.0000001;

/*

class GenotypeMutator {
  std::uniform_int_distribution<int> connections_dist;  // Note: no pointer
  std::bernoulli_distribution Mutation_Chance;

public:
  GenotypeMutator::GenotypeMutator(int upper_limit,float mu) : connections_dist(0,upper_limit);  Mutation_Chance(m){ }
  void mutate(std::vector<int>& genotype);
  

    // ...
};

void GenotypeMutator::mutate(std::vector<int> & genotype) {
  for(unsigned int base=0;base<genotype.size();++base) {
    if(Mutation_Chance(RNG_Engine)) {
      int previousFace=genotype[t];
      do {
        genotype[t]=Mutated_Colour(RNG_Engine);
      }while(genotype[t]==previousFace);
    }
  }      
}
*/
/*
//////////////
// MUTATIONS//
//////////////

class GenotypeMutator {
   public:
  void mutate(std::vector<int> genotype);
  GenotypeMutator(int c, double m);
 
   private:
  std::uniform_int_distribution<int> Mutated_Colours;
  std::bernoulli_distribution Mutation_Chance;

};
 
// Member functions definitions including constructor
void GenotypeMutator::GenotypeMutator(int c, int m) {
  std::uniform_int_distribution<int> Mutated_Colour(0,c);
  std::bernoulli_distribution Mutation_Chance(m); 
}
void GenotypeMutator::mutate(std::vector<int>& genotype) {
  for(unsigned int base=0;base<genotype.size();++base) {
    if(Mutation_Chance(RNG_Engine)) {
      int previousFace=genotype[t];
      do {
        genotype[t]=Mutated_Colour(RNG_Engine);
      }while(genotype[t]==previousFace);
    }
  }      
}
*/



//////////////////////////////////////
//FITNESS FUNCTION RELATED FUNCTIONS//
//////////////////////////////////////
double Fitness_Function(double Phenotype_Size) {
  double xi_scale=12.5;
  double eta_scale=4.;
  switch(Fitness_Mode) {
  case 0: return ((exp(xi_scale*Phenotype_Size)-1)/(exp(xi_scale)-1)+Phenotype_Size/eta_scale)/(1+1/eta_scale);
  case 1: return Phenotype_Size;
  case 4: return static_cast<int>(Phenotype_Size)==Target_Fitness ? Phenotype_Size : 0;
  default: return pow(Phenotype_Size,Fitness_Mode%10);
  }  
}

void Bump_Fitness(std::vector<double>& Fitnesses,bool up) {
  for(std::vector<double>::iterator fitness=Fitnesses.begin();fitness!=Fitnesses.end();++fitness) {
    if(up)
      *fitness=*fitness>11.1? *fitness*1000:*fitness/1000;
    else
      *fitness=*fitness>11.1? *fitness:0;
  }
}

std::vector<int> Random_Selection(int Num_Genomes,int K_Samples) {
  std::vector<int> Selected_Indicies(K_Samples);
  std::uniform_int_distribution<int> Random_Index(0,Num_Genomes-1);
  for(int n=0; n<K_Samples; ++n) {
    Selected_Indicies[n]=Random_Index(RNG_Engine);
  }
  return Selected_Indicies;
}

std::vector<int> Stochastic_Acceptance_Selection(std::vector<double>& Fitness_Weights,int K_Samples) {
  std::vector<int> Selected_Indicies(K_Samples);
  std::uniform_real_distribution<> Random_Interval(0, 1);
  std::uniform_int_distribution<int> Random_Index(0,Fitness_Weights.size()-1);
  double Max_Fitness=*std::max_element(Fitness_Weights.begin(),Fitness_Weights.end());
  if(Max_Fitness==0.0)
    return Random_Selection(Fitness_Weights.size(),K_Samples);
  for(int n=0; n<K_Samples; ++n) {
    bool Rejected_Selection=true;
    int index=0;
    while(Rejected_Selection) {
      index= Random_Index(RNG_Engine);
      if(Fitness_Weights[index]<MINIMUM_FITNESS_THRESHOLD)
        continue;
      if(Random_Interval(RNG_Engine)<Fitness_Weights[index]/Max_Fitness)
        Rejected_Selection=false;
    }
    Selected_Indicies[n]=index;
  }
  return Selected_Indicies;
}

std::vector<int> Roulette_Wheel_Selection(std::vector<double>& Fitness_Weights,int K_Samples) {
  std::vector<double> CDF(Fitness_Weights);
  std::vector<int> Selected_Indicies(K_Samples);
  double Total_Weight=std::accumulate(Fitness_Weights.begin(), Fitness_Weights.end(),0.0);
  if(Total_Weight<MINIMUM_FITNESS_THRESHOLD)
    return Random_Selection(Fitness_Weights.size(),K_Samples);
  for(std::vector<double>::iterator CDF_iter=CDF.begin()+1;CDF_iter!=CDF.end();++CDF_iter) {
    *CDF_iter+=*(CDF_iter-1);
  }
  std::uniform_real_distribution<> Random_Interval(0, Total_Weight-MINIMUM_FITNESS_THRESHOLD);
  for(int n=0; n<K_Samples; ++n) {
    unsigned int chosen=std::lower_bound(CDF.begin(),CDF.end(),Random_Interval(RNG_Engine))-CDF.begin();
    while(chosen>=Fitness_Weights.size() || Fitness_Weights[chosen]==0) {
      --chosen;
    }
    Selected_Indicies[n]=chosen;
  }
  return Selected_Indicies;
}


//Other

int Find_Percentile_of_Vector(std::vector<int>& vec,double percentile) {
  if(vec.size()==1) {
    return vec[0];
  }
  if(vec.size()==2) {
    return std::ceil(vec[0]/2.+vec[1]/2.);
  }
  std::vector<int>::iterator target=vec.begin()+std::ceil(vec.size()*percentile/100.);
  std::nth_element(vec.begin(),target,vec.end());
  if(vec.size()%2==1) {
    return *target;
  }
  else {
    std::vector<int>::iterator targetNeighbour=std::max_element(vec.begin(),target);
    return std::ceil((*targetNeighbour+*target)/2.);
  }                  
}

void Set_Runtime_Configurations(int argc, char* argv[]) {
  if(argc>3) {
    for(int arg=2;arg<argc;arg+=2) {
      switch(argv[arg][1]) {
        //BASIC PARAMETERS//
      case 'T': Num_Tiles=std::stoi(argv[arg+1]);
        break;
      case 'C': Colour_Space=std::stoi(argv[arg+1]);
        break;
      case 'N': Num_Genomes=std::stoi(argv[arg+1]);
        break;
      case 'K': GENERATION_LIMIT=std::stoi(argv[arg+1]);
        break;
      case 'M': Fitness_Mode=std::stoi(argv[arg+1]);
        break;     
     
        //DYNAMIC PARAMETERS//
      case 'O': Fitness_Oscillation_Rate=std::stoi(argv[arg+1]);
        break;
      case 'A': active_targets=std::stoi(argv[arg+1]);
        break;
      case 'U': mutation_cofactor=std::stoi(argv[arg+1]);
        break;

        //REGULATION PARAMETERS//
      case 'R': Regulated=std::stoi(argv[arg+1]);
        break;
      case 'I': initial_condition=std::stoi(argv[arg+1]);
        break;
        
        //RUN CONTROL PARAMETERS//
      case 'D': Num_Runs=std::stoi(argv[arg+1]);
        break;
      case 'V': RUN=std::stoi(argv[arg+1]);
        break;

        //OLD PARAMETERS//
      case 'S': Shape_Matching_Fitness=std::stoi(argv[arg+1]);
        break;
      case 'B': burn_in_period=std::stoi(argv[arg+1]);
        break;         
      case 'F': Target_Fitness=std::stoi(argv[arg+1]);
        break;
      default: std::cout<<"Unknown Parameter Flag: "<<argv[arg][1]<<std::endl;
      }
    }
  }
  std::cout<<"|||||||||||||||||||||||||||||||||||||"<<std::endl;
  std::cout<<"||Running with Following Parameters||"<<std::endl;
  std::cout<<"||Number of Tiles: "+std::to_string(Num_Tiles)+"               ||"<<std::endl;
  if(Colour_Space>9) {
    std::cout<<"||Number of Colours: "+std::to_string(Colour_Space)+"            ||"<<std::endl;
  }
  else {
    std::cout<<"||Number of Colours: "+std::to_string(Colour_Space)+"             ||"<<std::endl;
  }
  if(Target_Fitness>9) {
  std::cout<<"||Target Fitness: "+std::to_string(Target_Fitness)+"               ||"<<std::endl;
  }
  else {
    std::cout<<"||Target Fitness: "+std::to_string(Target_Fitness)+"                ||"<<std::endl;
  }
  std::cout<<"||Fitness Mode : "+std::to_string(Fitness_Mode)+"                 ||"<<std::endl;
  if(Num_Genomes>99) {
    std::cout<<"||Number of Genomes: "+std::to_string(Num_Genomes)+"           ||"<<std::endl;
  }
  else {
    std::cout<<"||Number of Genomes: "+std::to_string(Num_Genomes)+"            ||"<<std::endl;
  }
  if(Regulated) {
    std::cout<<"||Regulated: "<<std::boolalpha<<Regulated<<"                  ||"<<std::endl;
  }
  else {
    std::cout<<"||Regulated: "<<std::boolalpha<<Regulated<<"                 ||"<<std::endl;
  }
  std::cout<<"||Run: "+std::to_string(RUN)+"                           ||"<<std::endl;
  std::cout<<"|||||||||||||||||||||||||||||||||||||"<<std::endl;
}


void InitialGenotypeConditions(std::vector<int>& genotype) {
  std::uniform_int_distribution<int> Mutated_Colour(0,Colour_Space);
  int length_factor= Regulated ? 5:4;
  switch(initial_condition) {
    //MUTUAL//
  case 0:
  default: //zeroed
    genotype.assign(Num_Tiles*length_factor,0);
    if(Regulated)
      genotype[0]=1;
    break;   
  case 1: //all random
    for(int t=0;t<Num_Tiles*length_factor;++t) {
      genotype[t]=Mutated_Colour(RNG_Engine);
      if(Regulated && t%5==0)
	genotype[t]=genotype[t]>Colour_Space/2 ? 1 : 0;	
    }
    break;
  case 2: //all on, non-interacting
    genotype.assign(Num_Tiles*length_factor,1);
    break;
    //END MUTUAL//
    
    //REGULATION ONLY//
  case 20: //first on, random tiles 
  case 21: //all on, random tiles
    for(int t=0;t<Num_Tiles*length_factor;++t) {
      if(t%5==0) {
        if(t>0 && initial_condition==20)
          continue;
        genotype[t]=1;
      }
      else {
        genotype[t]=Mutated_Colour(RNG_Engine);
      }
    }
    break;

  case 22: //first on, non-interacting
    genotype.assign(Num_Tiles*length_factor,1);
    for(int t=5;t<Num_Tiles*length_factor;t+=5)
      genotype[t]=0;
    break;

  case 23: //all on, zeroed
    for(int t=0;t<Num_Tiles*length_factor;t+=5)
      genotype[t]=1;
    break;
    //END REGULATION ONLY//
  }
}
