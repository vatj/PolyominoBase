#include "stochastic_model.hpp"
#include <iostream>

extern "C" void WrappedGetPhenotypesID(const char* a,const char* b,bool file_of_genotypes,uint8_t colours,uint8_t n_genes);
extern "C" void GGenerator(const char* a,bool file_of_genotypes,uint8_t colours,uint8_t n_genes);
extern "C" void SampleMinimalGenotypes(const char* file_path_c, uint8_t n_genes, uint8_t colours,const uint32_t N_SAMPLES,bool allow_duplicates, bool file_of_genotypes);
extern "C" void GP_MapSampler(const char* file_path_c,uint8_t n_genes, uint8_t rcolours,uint8_t colours,bool file_of_genotypes);

std::vector<Phenotype_ID> GetPhenotypeIDs(Genotype& genotype, uint8_t k_builds, StochasticPhenotypeTable* pt_it);

uint64_t genotype_to_index(Genotype& genotype, uint8_t n_genes, uint8_t colours);
void index_to_genotype(uint64_t index, Genotype& genotype, uint8_t n_genes, uint8_t colours);



std::vector<Genotype> genotype_neighbourhood(const Genotype& genome, uint8_t ngenes, uint8_t colours);
void JiggleGenotype(Genotype& genotype, uint8_t max_colour);

struct NecklaceFactory {
  uint8_t colours=1;
  std::vector<std::vector<uint8_t> > necklaces;
  std::vector<uint8_t> necklace_grower;

  NecklaceFactory()  {necklace_grower.assign(5,0);}

  void GenNecklaces(uint8_t c) {
    colours=c;
    crsms_gen(1,1);
  }
  
  bool is_finite_necklace(std::vector<uint8_t>& neck) {
    //internal infinite loop
    if((neck[1] && Interaction_Matrix(neck[1])==neck[3]) || (neck[2] && Interaction_Matrix(neck[2])==neck[4]))
      return false;
    bool a_pair=false;
    for(uint8_t base=1; base<4;++base) {
      if(neck[base]==0)
	continue;
      uint8_t b1=std::count(neck.begin()+1,neck.end(),neck[base]), b2=std::count(neck.begin()+1,neck.end(),Interaction_Matrix(neck[base]));
      if(b1 && b2) {
	//internal branching point/degenerate double loop
	if(b1+b2>2)
	  return false;
	else {
	  //internal unique double loop
	  if(a_pair)
	    return false;
	  else
	    a_pair=true;
	}
      }
    }
    return true;
  }
  
  void is_necklace(uint64_t j) {
    if(4%j==0)
      if(is_finite_necklace(necklace_grower))
        necklaces.emplace_back(std::vector<uint8_t>{necklace_grower.begin()+1,necklace_grower.end()});
  }
  
  void crsms_gen(uint64_t n, uint64_t j) {
    if(n>4)
      is_necklace(j);
    else {
      necklace_grower[n]=necklace_grower[n-j];
      crsms_gen(n+1,j);
      for(uint64_t i=necklace_grower[n-j]+1;i<colours;++i) {
        necklace_grower[n]=i;
        crsms_gen(n+1,n);
      }
    }
  }
};

struct GenotypeGenerator {
  bool is_done=false;
  uint8_t n_genes,colours;
  std::vector<uint32_t> necklace_states;
  std::vector<std::vector<uint8_t> > necklaces;
  uint32_t n_necklaces;

  void init() {
    NecklaceFactory necks=NecklaceFactory();
    necks.GenNecklaces(colours);
    necklaces=necks.necklaces;
    n_necklaces=necklaces.size();
  }
  
  GenotypeGenerator(uint8_t a,uint8_t b) {n_genes=a;colours=b; necklace_states.assign(a,0);}

  Genotype operator() () {
    return !is_done ? next_genotype() : Genotype{};
  }

  bool valid_growing_faces(Genotype& genotype) {
    uint8_t max_face=1;
    for(auto face : genotype) {
      if(face>max_face)
        return false;
      if(face==max_face)
        max_face+=2;
    }
    return true;
  }

  bool valid_bindings(Genotype& genotype) {
    for(uint8_t interface=1;interface<=*std::max_element(genotype.begin(),genotype.end());interface+=2) {
      if(std::find(genotype.begin(),genotype.end(),interface)!=genotype.end()) { //is present
        if(std::find(genotype.begin(),genotype.end(),interface+1)==genotype.end()) //is not present
          return false;
      }
      else {
        if(std::find(genotype.begin(),genotype.end(),interface+1)!=genotype.end()) 
          return false;
      }
    }
    
    return true;
  }
  bool valid_genotype(Genotype& genotype) {
    if(!valid_growing_faces(genotype))
      return false;
    if(!valid_bindings(genotype))
      return false;
    return true;
  }
  
  void increment_states(std::vector<uint32_t>& states) {
    ++states.back();
    for(uint32_t rind=states.size();rind>0;--rind) {
      if(states[rind-1]>=n_necklaces) {
        if(rind==1) {
          is_done=true;
          return;
        }
        else { 
          states[rind-1]=0;
          ++states[rind-2];
        }
      }
    }
    auto max_iter=std::max_element(states.begin(),states.end());
    std::replace(max_iter,states.end(),static_cast<uint32_t>(0),*max_iter);
  }

  Genotype next_genotype() {
    Genotype genotype;
    while(!is_done) {
      genotype.clear();
      genotype.reserve(n_genes*4);
      for(auto index : necklace_states)
        genotype.insert(genotype.end(),necklaces[index].begin(),necklaces[index].end());
      increment_states(necklace_states);
      if(valid_genotype(genotype)) 
        return genotype;
    }   
    genotype.clear();
    return genotype;
  }
};
