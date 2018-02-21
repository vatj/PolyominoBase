#include <stochastic_model.hpp>

namespace Brute_Force
{
  std::random_device rd;
  std::mt19937 RNG_Generator(rd());
  
  std::vector<int> Brute_Force_Polyomino_Builder(std::vector<int> genome, unsigned int THRESHOLD_SIZE, int initial_Tile,int initial_Rotation) {
    
    std::vector<int> Placed_Tiles{0,0,initial_Tile,initial_Rotation}; //DEFINED AS (X,Y,Tile Type Number, Tile Rotation[in CW rotation])
    std::vector<int> Interacting_Faces; //DEFINED AS (X,Y,Target Face colour, Target Face Index) 
    for(int face=0; face<4; ++face) {
      if(genome[initial_Tile*4+ (face-initial_Rotation+4)%4]!=0)
        Brute_Force_Interacting_Adjacency(Interacting_Faces,genome[initial_Tile*4+ (face-initial_Rotation+4)%4],face,0,0);
    }
    
    while(!Interacting_Faces.empty()) {  
      for(int number_possibilities=(Interacting_Faces.size()/4)-1; number_possibilities >=0; --number_possibilities) {
        bool collision_Detected=false;
        for(std::vector<int>::iterator occupied_iter = Placed_Tiles.begin(); occupied_iter!=Placed_Tiles.end(); occupied_iter+=4) {
          if(Interacting_Faces[number_possibilities*4]==*occupied_iter && Interacting_Faces[number_possibilities*4+1]==*(occupied_iter+1)) {
            if(Interacting_Faces[number_possibilities*4+2]!=genome[*(occupied_iter+2)*4+(Interacting_Faces[number_possibilities*4+3]-*(occupied_iter+3)+4)%4])
              return {}; //Steric mismatch, reject empty vector as sign
            Interacting_Faces.erase(Interacting_Faces.end()-4,Interacting_Faces.end()); 
            collision_Detected=true;
            break;
          }
        }
        if(collision_Detected)
          continue;
        else {
          int conjugate_count=std::count(genome.begin(),genome.end(),Interacting_Faces[number_possibilities*4+2]);
          std::uniform_int_distribution<int> Random_Count(0,conjugate_count-1);
          int nth_conjugate=Random_Count(RNG_Generator);
          std::vector<int>::iterator current_conjugate=std::find(genome.begin(),genome.end(),Interacting_Faces[number_possibilities*4+2]);
          for(int conj_cnt=1;conj_cnt<=nth_conjugate;++conj_cnt) {
            current_conjugate=std::find(current_conjugate+1,genome.end(),Interacting_Faces[number_possibilities*4+2]);
          }
          int new_Tile=(current_conjugate-genome.begin())/4;
          int new_Face=(current_conjugate-genome.begin())%4;
          int rotation= (Interacting_Faces[number_possibilities*4+3]-new_Face+4)%4;
          Placed_Tiles.insert(Placed_Tiles.end(),{Interacting_Faces[number_possibilities*4],Interacting_Faces[number_possibilities*4+1],new_Tile,rotation});
          if(Placed_Tiles.size()/4 >THRESHOLD_SIZE) 
            return Placed_Tiles;
          int placed_X=Interacting_Faces[number_possibilities*4];
          int placed_Y=Interacting_Faces[number_possibilities*4+1];
          Interacting_Faces.erase(Interacting_Faces.begin()+number_possibilities*4,Interacting_Faces.begin()+number_possibilities*4+4);
          for(int face=1;face<4;++face) {
            int temp_Face=genome[new_Tile*4+(new_Face+face)%4];
            if(temp_Face!=0)
              Brute_Force_Interacting_Adjacency(Interacting_Faces,temp_Face,(new_Face+face+rotation)%4,placed_X,placed_Y);
          }
          break;
        }
      }
    }
    return Placed_Tiles;
  }

  void Brute_Force_Interacting_Adjacency(std::vector<int>& Interacting_Faces, int interacting_Face, int face_index, int X, int Y) {
    int X_OFFSET=0,Y_OFFSET=0;
    switch(face_index) {
    case 0:
      X_OFFSET=0;
      Y_OFFSET=1;
      break;
    case 1:
      X_OFFSET=1;
      Y_OFFSET=0;
      break;
    case 2:
      X_OFFSET=0;
      Y_OFFSET=-1;
      break;
    case 3:
      X_OFFSET=-1;
      Y_OFFSET=0;
      break;
    }
    int conjugate_Face=Interaction_Matrix(interacting_Face);//(1-interacting_Face%2)*(interacting_Face-1)+(interacting_Face%2)*(interacting_Face+1); 
    std::uniform_int_distribution<int> Random_Insertion(0,Interacting_Faces.size()/4);
    Interacting_Faces.insert(Interacting_Faces.begin()+Random_Insertion(RNG_Generator)*4,{X+X_OFFSET,Y+Y_OFFSET,conjugate_Face,(face_index+2)%4});
  }

  int Analyse_Genotype_Outcome(std::vector<int> genome, int N_Repeated_Checks) {
    Clean_Genome(genome,-1); 
    const unsigned int THRESHOLD_SIZE=(genome.size()*genome.size())/4;
    bool UND_Check=false;
    std::vector<int> Placed_Tiles_Check=Brute_Force_Polyomino_Builder(genome,THRESHOLD_SIZE,0,0);
    
    if(Placed_Tiles_Check.empty()) //STERIC
      return -1;
    if(Placed_Tiles_Check.size()/4 > THRESHOLD_SIZE)
      return -1;
    
    int Delta_X_Check=-1,Delta_Y_Check=-1;
    std::vector<int> Spatial_Occupation_Check=Generate_Spatial_Occupancy(Placed_Tiles_Check,Delta_X_Check,Delta_Y_Check,3);    
    //std::vector<int> Spatial_Tile_Check=Generate_Spatial_Occupancy(Placed_Tiles_Check,Delta_X_Check,Delta_Y_Check,1);
    //std::vector<int> Spatial_Orientation_Check=Generate_Spatial_Occupancy(Placed_Tiles_Check,Delta_X_Check,Delta_Y_Check,2);
    //Declare but not initialise
    std::vector<int> Placed_Tiles_Compare, Spatial_Occupation_Compare;//, Spatial_Tile_Compare,Spatial_Orientation_Compare;
    
    bool broken_Comparison=false;
    for(int nth_repeat=1;nth_repeat<N_Repeated_Checks;++nth_repeat) {
      Placed_Tiles_Compare=Brute_Force_Polyomino_Builder(genome,THRESHOLD_SIZE,nth_repeat%(genome.size()/4),0);
      if(Placed_Tiles_Compare.empty()) //STERIC
        return -1;
      if(Placed_Tiles_Compare.size()/4 > THRESHOLD_SIZE)
        return -1;
      if(Placed_Tiles_Check.size()!=Placed_Tiles_Compare.size())
        return -5;
      
      int Delta_X_Compare=-1,Delta_Y_Compare=-1;
      Spatial_Occupation_Compare=Generate_Spatial_Occupancy(Placed_Tiles_Compare,Delta_X_Compare,Delta_Y_Compare,3);
      /*
      std::cout<<"A "<<Delta_X_Check<<","<<Delta_Y_Check<<" : ";
        for(auto x:Spatial_Occupation_Check)
          std::cout<<x<<" ";
        std::cout<<"\nB "<<Delta_X_Compare<<","<<Delta_Y_Compare<<" : ";
        for(auto x:Spatial_Occupation_Compare)
          std::cout<<x<<" ";
        std::cout<<"\n";
        
        Clockwise_Pi_Rotation(Spatial_Occupation_Compare,Delta_X_Compare,Delta_Y_Compare);
        std::cout<<"\nC "<<Delta_X_Compare<<","<<Delta_Y_Compare<<" : ";
        for(auto x:Spatial_Occupation_Compare)
          std::cout<<x<<" ";
        std::cout<<"\n";
        Clockwise_Rotation(Spatial_Occupation_Compare,Delta_X_Compare,Delta_Y_Compare);
        std::swap(Delta_X_Compare,Delta_Y_Compare);
        std::cout<<"\nD "<<Delta_X_Compare<<","<<Delta_Y_Compare<<" : ";
        for(auto x:Spatial_Occupation_Compare)
          std::cout<<x<<" ";
        std::cout<<"\n";

        Clockwise_Rotation(Spatial_Occupation_Compare,Delta_X_Compare,Delta_Y_Compare);
        std::swap(Delta_X_Compare,Delta_Y_Compare);
        std::cout<<"\nE "<<Delta_X_Compare<<","<<Delta_Y_Compare<<" : ";
        for(auto x:Spatial_Occupation_Compare)
          std::cout<<x<<" ";
        std::cout<<"\n";
        */
      
      if(!Compare_Two_Polyominoes_Shapes(Spatial_Occupation_Check,Delta_X_Check,Delta_Y_Check,Spatial_Occupation_Compare,Delta_X_Compare,Delta_Y_Compare)) {
        
        return -7;
        broken_Comparison=true;
        break;
      }
      /*
      else {
        Spatial_Tile_Compare=Generate_Spatial_Occupancy(Placed_Tiles_Compare,Delta_X_Compare,Delta_Y_Compare,1);
        if(!Compare_Two_Polyominoes_Shapes(Spatial_Tile_Check,Delta_X_Check,Delta_Y_Check,Spatial_Tile_Compare,Delta_X_Compare,Delta_Y_Compare)) {                      
          broken_Comparison=true;
          break;
        }
        else {
          Spatial_Orientation_Compare=Generate_Spatial_Occupancy(Placed_Tiles_Compare,Delta_X_Compare,Delta_Y_Compare,2);
          if(!Compare_Two_Polyominoes_Shapes(Spatial_Orientation_Check,Delta_X_Check,Delta_Y_Check,Spatial_Orientation_Compare,Delta_X_Compare,Delta_Y_Compare)) {
            return -6;
          }
        
        }
      }
      */
      if(N_Repeated_Checks-nth_repeat>1) {
        Placed_Tiles_Check=Placed_Tiles_Compare;
        Spatial_Occupation_Check=Spatial_Occupation_Compare;
        //Spatial_Tile_Check=Spatial_Tile_Compare;
        //Spatial_Orientation_Check=Spatial_Orientation_Compare;
        Delta_X_Check=Delta_X_Compare;
        Delta_Y_Check=Delta_Y_Compare;
      }
    }    
    if(UND_Check) {
      return -5; 
    }
    else { //Bound
      if(broken_Comparison) { //BND
        return -5;
      }
      else {
        return Placed_Tiles_Check.size()/4;//Steric_Check(genome);
      }
    }
  }
  
  std::vector<int> Generate_Spatial_Occupancy(std::vector<int>& Placed_Tiles_Check, int& DELTA_X_Check,int& DELTA_Y_Check, int generate_mode) {
    std::vector<int> X_Locs_Check, Y_Locs_Check, Tile_Type_Check,Tile_Orientation_Check;
    for(std::vector<int>::iterator check_iter = Placed_Tiles_Check.begin();check_iter!=Placed_Tiles_Check.end();check_iter+=4) {
      X_Locs_Check.emplace_back(*check_iter);
      Y_Locs_Check.emplace_back(*(check_iter+1));
      Tile_Type_Check.emplace_back(*(check_iter+2));
      Tile_Orientation_Check.emplace_back(*(check_iter+3));
    }
    std::vector<int>::iterator LEFT_X_Check,RIGHT_X_Check,TOP_Y_Check,BOTTOM_Y_Check;
    std::tie(LEFT_X_Check,RIGHT_X_Check)=std::minmax_element(X_Locs_Check.begin(),X_Locs_Check.end());
    std::tie(BOTTOM_Y_Check,TOP_Y_Check)=std::minmax_element(Y_Locs_Check.begin(),Y_Locs_Check.end());
    DELTA_X_Check=*RIGHT_X_Check-*LEFT_X_Check+1;
    DELTA_Y_Check=*TOP_Y_Check-*BOTTOM_Y_Check+1;
    std::vector<int> Spatial_Occupancy_Check(DELTA_X_Check*DELTA_Y_Check);

    for(unsigned int tileIndex=0;tileIndex<X_Locs_Check.size();++tileIndex) {
      if(generate_mode==0) {
        Spatial_Occupancy_Check[(*TOP_Y_Check-Y_Locs_Check[tileIndex])*DELTA_X_Check + (X_Locs_Check[tileIndex]-*LEFT_X_Check)]=1;
      }
      if(generate_mode==1) {
        Spatial_Occupancy_Check[(*TOP_Y_Check-Y_Locs_Check[tileIndex])*DELTA_X_Check + (X_Locs_Check[tileIndex]-*LEFT_X_Check)]=Tile_Type_Check[tileIndex];
      }
      if(generate_mode==2) {
        Spatial_Occupancy_Check[(*TOP_Y_Check-Y_Locs_Check[tileIndex])*DELTA_X_Check + (X_Locs_Check[tileIndex]-*LEFT_X_Check)]=Tile_Orientation_Check[tileIndex];
      }
      if(generate_mode==3) {
        Spatial_Occupancy_Check[(*TOP_Y_Check-Y_Locs_Check[tileIndex])*DELTA_X_Check + (X_Locs_Check[tileIndex]-*LEFT_X_Check)]=1+Tile_Type_Check[tileIndex]*4+Tile_Orientation_Check[tileIndex];
      } 
    }
    return Spatial_Occupancy_Check;
  }
  int Compare_Two_Polyominoes_Tile_Details(std::vector<int>& Placed_Tiles_Check, std::vector<int>& Placed_Tiles_Compare) {
    //return Codes
    //1 - Not tile deterministic
    //2 - Tile, but not orientation determinsitic
    //3 - Tile and orientation determinstic 
    int return_code=3;
    for(std::vector<int>::iterator check_iter = Placed_Tiles_Check.begin(); check_iter!=Placed_Tiles_Check.end(); check_iter+=4) {
      for(std::vector<int>::iterator compare_iter = Placed_Tiles_Compare.begin(); compare_iter!=Placed_Tiles_Compare.end(); compare_iter+=4) {
        if(*check_iter==*compare_iter && *(check_iter+1) == *(compare_iter+1)) {
          if(*(check_iter+2)!=*(compare_iter+2)) {
            return 1;
          }
          if(*(check_iter+3)!=*(compare_iter+3)) {
            return_code=2;
          }
        }
      }
    }
    return return_code;
  }
 
  bool Compare_Two_Polyominoes_Shapes(std::vector<int>& Spatial_Occupation_Check,int Delta_X_Check,int Delta_Y_Check, std::vector<int>& Spatial_Occupation_Compare,int Delta_X_Compare,int Delta_Y_Compare) {
    if(std::count_if(Spatial_Occupation_Check.begin(),Spatial_Occupation_Check.end(),[](int i){return i!=0;})!=std::count_if(Spatial_Occupation_Compare.begin(),Spatial_Occupation_Compare.end(),[](int i){return i!=0;})) {
      std::cout<<"here"<<std::endl;
      return false;
    }
    if(Delta_X_Check==Delta_X_Compare && Delta_Y_Check==Delta_Y_Compare && Delta_X_Check==Delta_Y_Check) { //bounding boxes match, symmetric
      if(Spatial_Occupation_Check==Spatial_Occupation_Compare) {
        return true;
      }
      else {
        for(int rotation=0;rotation<3;++rotation) {
          Clockwise_Rotation(Spatial_Occupation_Check,Delta_X_Check,Delta_Y_Check);
          if(Spatial_Occupation_Check==Spatial_Occupation_Compare) {
            return true;
          }
        }
        return false;
      }
    }
    if(Delta_X_Check==Delta_X_Compare && Delta_Y_Check==Delta_Y_Compare) { //bounding boxes match, asymmetric
      if(Spatial_Occupation_Check==Spatial_Occupation_Compare) {
        return true;
      }
      else {
        Clockwise_Pi_Rotation(Spatial_Occupation_Check,Delta_X_Check,Delta_Y_Check);
        if(Spatial_Occupation_Check==Spatial_Occupation_Compare) {
          return true;
        }
        return false;
      }
    }
    if(Delta_X_Check==Delta_Y_Compare && Delta_Y_Check==Delta_X_Compare) { //bounding boxes pi/2 off, asymmetric
      Clockwise_Rotation(Spatial_Occupation_Check,Delta_X_Check,Delta_Y_Check);
      std::swap(Delta_X_Check,Delta_Y_Check);
      if(Spatial_Occupation_Check==Spatial_Occupation_Compare) {
        return true;
      }
      else {
        Clockwise_Pi_Rotation(Spatial_Occupation_Check,Delta_X_Check,Delta_Y_Check);
        if(Spatial_Occupation_Check==Spatial_Occupation_Compare) {
          return true;
        }
        return false;
      }
    }
    //nomiminally else, completely mismatch
    return false;
  }
}

