#include<bits/stdc++.h>

using namespace std;

//Combining Graph_Node and Network
class Network
{
private:
	vector<string> node_name;	//Stores the names of the nodes
	int num_nodes;
	map<string, int> node_id;	//Maps the name to it's index for each node
	vector<vector<string> > values;	//Categories of possible values
	vector<int> numvalues;	//Stores the total number of values for each node
	vector<vector<int> > children;	//Will store the index of all children of all nodes
	vector<vector<int> > parents;	//Will store the index of all parents of all nodes
	vector<vector<double> > CPT;	//CPT for each node

	vector<vector<int>> data;	//Data is stored as integer format. Suppose a node can take 3 categories as values. Then first category is '0', then '1' and so on.
	vector<int> missing;	//Stores index of question mark for data i

	vector<vector<int> > datapoints;	//Stores the useful data, after replacing '?' with possible values
	vector<double> weights;	//Weights for the data that we will be using

public:
	Network()
	{
		//We are guaranteed that there will be 37 nodes, so to save time while push_back by resizing
		num_nodes = 0;
		node_name.resize(37);
		numvalues.resize(37);
		children.resize(37);
		parents.resize(37);
		CPT.resize(37);
		values.resize(37);
	}
	void read_network(string filename)
	{
		ifstream myfile(filename); 
		string line, temp, name;
		if(myfile.is_open())
		{
			while(!myfile.eof())
			{
				stringstream ss;
				getline(myfile,line);
				ss.str(line);
				ss>>temp;
				if(temp.compare("variable")==0)
				{	
					ss>>name;
					node_name[num_nodes] = name;
					node_id[name] = num_nodes;
					getline(myfile,line);  
					stringstream ss2;
					ss2.str(line);
					for(int i=0;i<4;i++)
					{
						ss2>>temp;
					}
					while(temp.compare("};")!=0)
					{
						values[num_nodes].push_back(temp);
						ss2>>temp;
					}
					numvalues[num_nodes] = values[num_nodes].size();
					num_nodes++;
				}
				else 
				if(temp.compare("probability")==0)
				{
					ss>>temp;
					ss>>temp;
					int cur_idx=node_id[temp];
					ss>>temp;
					while(temp.compare(")")!=0)
					{
						int parent_idx = node_id[temp];
						children[parent_idx].push_back(cur_idx);
						parents[cur_idx].push_back(parent_idx);
						ss>>temp;
					}
					getline (myfile,line);
					stringstream ss2;
					ss2.str(line);
					ss2>>temp;
					ss2>>temp;
					vector<double> curr_CPT;
					while(temp.compare(";")!=0)
					{
						CPT[cur_idx].push_back(atof(temp.c_str()));
						ss2>>temp;
					}
				}
			}
			myfile.close();
		}
	}

	void init_CPT()
	{
		for(int i = 0; i < num_nodes; i++)
		{
			for(int j = 0; j < CPT[i].size(); j++)
			{
				CPT[i][j] = 1.0/numvalues[i];
			}
		}
	}

	int value_idx(int cur_idx, string category)
	{
		for(int i = 0; i < values[cur_idx].size(); i++)
		{
			if(values[cur_idx][i] == category)
				return i;
		}
		return -1;	//This handles the case of inserting value_idx for "?" as well!
	}

	void read_data(string filename)
	{
		ifstream myfile(filename);
		string vals, line;
		while(!myfile.eof())
		{
			stringstream ss;
			bool flag = false;
			getline(myfile, line);
			int counter = 0;
			ss.str(line);
			vector<int> temp;
			while(ss>>vals)
			{
				if(vals == "\"?\"")
				{
					missing.push_back(counter);
					flag = true;
				}
				temp.push_back(value_idx(counter, vals)); //For "?", pushes -1
				counter++;
			}
			if(!flag)
				missing.push_back(-1);
			data.push_back(temp);

			if(!flag)
				datapoints.push_back(temp);
			else
			{
				int missing_idx = missing[missing.size()-1];
				int N = numvalues[missing_idx];
				for(int t = 0; t < N; t++)
				{
					datapoints.push_back(temp);
					datapoints[datapoints.size()-1][missing_idx] = t;
				}
			}
			
		}
		myfile.close();
	}

	int get_CPT_Index(vector<int> &vals, vector<int> &sizes)
	{
		if(vals.size() == 0)
		{
			return 0;
		}
		int idx = 0, b=1;
		int M = sizes.size();
		for(int i = M-1; i>=0; i--)
		{
			idx = idx + b*vals[i];
			b = b*sizes[i];
		}
		return idx;
	}

	void expectation()
	{
		weights.clear();
		for(int i = 0; i < data.size(); i++)
		{
			int missing_idx = missing[i];	//This node has the missing value
			if(missing_idx == -1)
				weights[i] = 1;
			else
			{
				int N = numvalues[missing_idx];
				double numerator, denominator = 0.0;	//Will be a constant
				vector<double> all_nums;
				for(int t = 0; t < N; t++)
				{
					numerator = 1.0;
					vector<int> temp(data[i].begin(), data[i].end());
					temp[missing_idx] = t;
					vector<int> vals, sizes;
					for(int j = 0; j < children[missing_idx].size(); j++)
					{
						vals.clear(), sizes.clear();
						vals.push_back(data[i][children[missing_idx][j]]);
						sizes.push_back(numvalues[children[missing_idx][j]]);
						for(int k = 0; k < parents[children[missing_idx][j]].size(); k++)
						{
							sizes.push_back(numvalues[parents[children[missing_idx][j]][k]]);
							vals.push_back(temp[parents[children[missing_idx][j]][k]]);
						}
						numerator = numerator * CPT[children[missing_idx][j]][get_CPT_Index(vals, sizes)];
					}	
					denominator += numerator; 			//Denominator = P(child(X)|parents of X except X) so add all up, for all values of X
					vals.clear(), sizes.clear();
					vals.push_back(t);
					sizes.push_back(N);
					for(int j = 0; j < parents[missing_idx].size(); j++)
					{
						sizes.push_back(numvalues[parents[missing_idx][j]]);
						vals.push_back(temp[parents[missing_idx][j]]);
					} 
					numerator = numerator * CPT[missing_idx][get_CPT_Index(vals, sizes)];
					all_nums.push_back(numerator);
				}
				for(int j = 0; j < all_nums.size(); j++)
				{
					weights.push_back(all_nums[j]/denominator);
				}	
			}
		}
	}

	bool maximization()
	{
		double max_diff = 0.0;
		for(int i = 0; i < CPT.size(); i++)
		{
			vector<int> vals, vals_idx, sizes;
			vals_idx.push_back(i);
			sizes.push_back(numvalues[i]);
			for(int j = 0; j < parents[i].size(); j++)
			{
				vals_idx.push_back(parents[i][j]);
				sizes.push_back(numvalues[parents[i][j]]);
			}
			//Parents have been stored. Compute denominator and numerator
			int MOD = CPT[i].size()/numvalues[i];
			vector<double> denominators(MOD, 0.0), numerators(CPT[i].size(), 0.0);
			//Now, start editing the CPT
			for(int j = 0; j < datapoints.size(); j++)
			{
				vals.clear();
				int index;

				for(int k = 0; k < vals_idx.size(); k++)
				{
					vals.push_back(datapoints[j][vals_idx[k]]);
				}

				index = get_CPT_Index(vals, sizes);

				denominators[index%MOD] += weights[j];
				numerators[index] += weights[j];
			}
			//Finally update CPT;
			double temp;
			for(int j = 0; j < CPT[i].size(); j++)
			{
				temp = (numerators[j]+0.001)/(denominators[j%MOD]+0.001*numvalues[i]);
				max_diff = max(max_diff, abs(temp-CPT[i][j]));
				CPT[i][j] = temp;
				if(CPT[i][j] < 0.0001)
					CPT[i][j] = 0.0001;	//Addition for smoothing and avoiding a zero
			}
		}
		
		if(max_diff < 0.0001)
			return true;
		return false;
	}

	void write_network(string filename)
	{
		ifstream myfile(filename); 
		ofstream outfile;
		outfile.open("solved_alarm.bif");
		outfile<<setprecision(4)<<fixed;
		string line, temp, name;
		int counter = 0;
		if(myfile.is_open())
		{
			while(!myfile.eof())
			{
				stringstream ss;
				getline(myfile,line);
				ss.str(line);
				ss>>temp;
				if(temp.compare("probability")==0)
				{
					outfile<<line<<"\n";
					getline (myfile,line);
					outfile<<"\ttable ";
					for(int i = 0; i < CPT[counter].size(); i++)
						outfile<<CPT[counter][i]<<" ";
					outfile<<" ;\n";
					counter++;
				}
				else
				{
					if(line.size()!=0)
						outfile<<line<<"\n";
					else
						outfile<<line;
				}
			}
			myfile.close();
			outfile.close();
		}
	}
};

int main(int argc, char** argv)
{
	time_t start = time(NULL);
	Network med_diagnosis;
	med_diagnosis.read_network(argv[1]);
	med_diagnosis.init_CPT();
	med_diagnosis.read_data(argv[2]);
	while(true)
	{
		med_diagnosis.expectation();
		bool done = med_diagnosis.maximization();
		if(done)	//Run deterministally for 5 minutes
			break;	
	}
	med_diagnosis.write_network(argv[1]);
	return 0;
}


