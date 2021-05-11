#include <iostream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <sstream>
#include <fstream>

using namespace std;

// Abstract class
class Employee
{
	protected:

		std::string Surname;
		std::string Firstname;
		int day;
		int month;
		int year;
		
		const int monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	public:

		// Constructor
		Employee(std::string s_name, std::string f_name, int d, int m, int y)
		{
			Surname = s_name;
			Firstname = f_name;
			day = d;
			month = m;
			year = y;
		}

		// Create database
		void CreateDatabase()
		{

			fstream fout;
			fout.open("database.csv", ios::out);
			fout << "Surname\tFirstname\tBirth date\tType\t";
			fout << "Total leave\t";
			fout << "Taken leave\n";

			fout.close();
		}

		// Count leave 
		int AddLeaves()
		{
			int cd = 1;
			int cm = 1;
			int cy = 2021;

			if (day > cd)
			{
				cd += monthDays[day - 1];
				cd -= 1;
			}

			if (month > cm) 
			{
				cy -= 1;
				cm += 12;
			}

			if((cy - year) >= 50)
				return 32;
			else
				return 30;
		}

		// Apply leave from date to to date
		int ApplyLeave(string from, string to)
		{

    			int fd, fm, fy;
    			int td, tm, ty;

			stringstream ss;
    			ss << from;

			ss >> fd;
    			ss.ignore(1); 
    			ss >> fm;
    			ss.ignore(1);  
    			ss >> fy;

			stringstream ss1;
    			ss1 << to;

			ss1 >> td;
    			ss1.ignore(1); 
    			ss1 >> tm;
    			ss1.ignore(1);  
    			ss1 >> ty;

			if(!((fy >= 2021) && (ty <= 2022)))
			{
				cout<<"Please apply leave between 01/01/2021 to 01/01/2022"<<endl;
				return -1;
			}

			if (fm < 3)
			{
        			fy--;
				fm += 12;
			}

		   	int d1 = 365*fy + fy/4 - fy/100 + fy/400 + (153*fm - 457)/5 + fd - 306;

			if (tm < 3)
			{
        			ty--;
				tm += 12;
			}
    			
			int d2 = 365*ty + ty/4 - ty/100 + ty/400 + (153*tm - 457)/5 + td - 306;

    			return (d2 - d1);
		}

		// Serch employee using surname and first name
		int SerchEmployee(string surname, string firstname)
		{
			bool isUsrExist = false;
			string line;
			ifstream file;
			file.open("database.csv");
	
			while (getline(file, line))
    			{
        			stringstream str(line);
        			string sn;	
        			string fn;
				str>>sn>>fn;

				if((firstname.compare(fn)) == 0)
				{
					cout<<line<<endl;
					isUsrExist = true;
				}
        		}
			if(!isUsrExist)
				cout<<"Employee is not exist"<<endl;

			file.close();
		}

		// Update Leave in database
		int UpdateLeave(string surname, string firstname, int leave)
		{
			int cnt = 0;
			string line;
			ifstream file;
			file.open("database.csv");

        		string sn, fn, bd, ty;	
			int tl, tkl;

			while (getline(file, line))
    			{
				cnt++;
        			stringstream str(line);
				str>>sn>>fn>>bd>>ty>>tl>>tkl;

				if((firstname.compare(fn)) == 0)
				{
					break;
				}
        		}

			if(tl < leave)
			{
				cout<<"You have "<<tl<<" leave. You can't apply more than that."<<endl;
				return 0;
			}
		
			int l = 0;
			file.clear();
			file.seekg(0);

			ofstream out;
			out.open("temp.csv",ios::app);

			while (getline(file, line))
			{
				l++;
				if(cnt != l)
					out<<line<<"\n";
				else
				{
					int t = tl - leave;
					out<<sn<<"\t"<<fn<<"\t"<<bd<<"\t";
					out<<ty<<"\t"<<t<<"\t"<<leave<<"\n";
				}
			}
			file.close();
			out.close();
			remove("database.csv");
			rename("temp.csv","database.csv");
			
			file.close();
		}

		// Delete employee from database
		int DeleteEmployee(string surname, string firstname)
		{
			int cnt = 0;
			bool isUsrExist = false;
			string line;
			ifstream file;
			file.open("database.csv");

			while (getline(file, line))
    			{
				cnt++;
        			stringstream str(line);
        			string sn;	
        			string fn;
				str>>sn>>fn;

				if((firstname.compare(fn)) == 0)
				{
					isUsrExist = true;
					break;
				}
        		}
			if(!isUsrExist)
				cout<<"Employee is not exist"<<endl;
			else
			{
				int l = 0;
				file.clear();
				file.seekg(0);

				ofstream out;
				out.open("temp.csv",ios::app);

				while (getline(file, line))
				{
					l++;
					if(cnt != l)
						out<<line<<"\n";
				}
				file.close();
				out.close();
				remove("database.csv");
				rename("temp.csv","database.csv");
			}

			file.close();
		}

		// Display all emplyees details
		int DisplayEmployee()
		{
			string line;
			ifstream file;
			file.open("database.csv");

			//getline(file, line);
			while (getline(file, line))
    			{
        			cout<<line<<endl;
        		}

			file.close();
		}

		// Get first name of employee
		string getFirstname(Employee *e)
		{
			return (e->Firstname);
		}
		
		// Pure virtual function
		virtual void AddEmployee() = 0;

};


// Class for Hourly Employee 
class HourlyEmployee : public Employee
{
	private:
		int total_leave;
		int taken_leave;
		int hours;
		int hourly_wage;
		long long total_wages;

	public:

		// Constructor
		HourlyEmployee(string sn, string fn, int d, int m, int y):Employee(sn, fn, d, m, y)
		{

		}

		// Add employe detail in database
		void AddEmployee()
		{
			cout<<"Enter hours"<<endl;
			cin>>hours;
			
			cout<<"Enter hourly wage"<<endl;
			cin>>hourly_wage;

			total_wages = (hours * hourly_wage);

			total_leave = AddLeaves();

			fstream fout;

			fout.open("database.csv", ios::out | ios::app);
			fout << Surname<<"\t";
			fout << Firstname<<"\t\t";
			fout << day<<"/";
			fout << month<<"/";
			fout << year<<"\t";
			fout << "HE\t";
			fout << total_leave <<"\t\t";
			fout << taken_leave <<"\t\t";
			fout << total_wages <<"\n";
			fout.close();
		}
};

// Class for Salaried Employee
class SalariedEmployee : public Employee
{
	private:
		int total_leave;
		int taken_leave;
		long long annualy_salary;
	public:

		// Constructor
		SalariedEmployee(string sn, string fn, int d, int m, int y):Employee(sn, fn, d, m, y)
		{

		}

		// Add employee
		void AddEmployee()
		{
			total_leave = AddLeaves();
			taken_leave = 0;
			cout<<"Enter annual salary"<<endl;
			cin>>annualy_salary;
			cout<<annualy_salary<<"\n";
			fstream fout;

			fout.open("database.csv", ios::out | ios::app);
			fout << Surname<<"\t";
			fout << Firstname<<"\t";
			fout << day<<"/";
			fout << month<<"/";
			fout << year<<"\t";
			fout << "SE\t";
			fout << total_leave <<"\t";
			fout << taken_leave <<"\t";
			fout << annualy_salary <<"\n";
			fout.close();
		}
};

// Class for manager
class Manager : public Employee
{
	private:
		int total_leave;
		int taken_leave;
		//long long annualy_salary;
	public:

		// constructor
		Manager(string sn, string fn, int d, int m, int y):Employee(sn, fn, d, m, y)
		{

		}

		// Add detail in database
		void AddEmployee()
		{
			total_leave = AddLeaves();
			taken_leave = 0;
			//cout<<"Enter annual salary"<<endl;
			//cin>>annualy_salary;
			//cout<<annualy_salary<<"\n";
			fstream fout;

			fout.open("database.csv", ios::out | ios::app);
			fout << Surname<<"\t";
			fout << Firstname<<"\t";
			fout << day<<"/";
			fout << month<<"/";
			fout << year<<"\t";
			fout << "SE\t";
			fout << total_leave <<"\t";
			fout << taken_leave <<"\n";
			//fout << annualy_salary <<"\n";
			fout.close();
		}
};


// Main entry point
int main() 
{
	Employee *employees[500];
	int numEmployees = 0;

	int option = 0;
	string line, code, dataStr;

	Employee *ed;
	ed->CreateDatabase();

	while (true) 
	{

		cout<<"Please select option"<<endl;
		cout<<"1) Add employee\n";
		cout<<"2) Apply leave\n";
		cout<<"3) Serch employee\n";
		cout<<"4) Display employees\n";
		cout<<"5) Delete employee\n";
		cout<<"6) Exit\n";

		cin>>option;

		switch(option)
		{
			case 1: // Add employee
				{
					string surname;
					string firstname;
					int type;
					string birthdate;

					cout<<"Enter surname"<<endl;
					cin>>surname;

					cout<<"Enter firstname"<<endl;
					cin>>firstname;

					cout<<"Enter type ( 1)Hourly employee, 2) Salaried employee and 3) Manager)"<<endl;
					cin>>type;

					cout<<"Enter birthdate (dd/mm/yy)"<<endl;
					cin>>birthdate;

					stringstream ss;
    					ss << birthdate;

    					int day, month, year;

					ss >> day;
    					ss.ignore(1); 
    					ss >> month;
    					ss.ignore(1);  
    					ss >> year;

					if(type == 1)
					{
						employees[numEmployees] = new HourlyEmployee(surname, firstname, day, month, year);
						employees[numEmployees]->AddEmployee();

						numEmployees++;
					}
					else if(type == 2)
					{
						employees[numEmployees] = new SalariedEmployee(surname, firstname, day, month, year);
						employees[numEmployees]->AddEmployee();
						numEmployees++;
					}
					else if(type == 3)
					{
						employees[numEmployees] = new Manager(surname, firstname, day, month, year);
						employees[numEmployees]->AddEmployee();
						numEmployees++;
					}
					break;
				}	
			case 2: //Apply leave
				{
					string surname;
					string firstname;
					string from;
					string to;
					int id;

					bool isEmployeeExist = false;

					cout<<"Enter surname"<<endl;
					cin>>surname;

					cout<<"Enter firstname"<<endl;
					cin>>firstname;

					for(int i = 0; i < numEmployees; i++)
					{
						string name = employees[i]->getFirstname(employees[i]);
						if((firstname.compare(name)) == 0)
						{
							id = i;
							isEmployeeExist = true;
						}
					}

					if(isEmployeeExist)
					{
						cout<<"From date(dd/mm/yy)"<<endl;
						cin>>from;

						cout<<"To date(dd/mm/yy)"<<endl;
						cin>>to;

						int leave = employees[id]->ApplyLeave(from, to);
						if(leave != -1)
						{
							cout<<"Applied "<<leave+1<<" leave"<<endl;
							employees[id]->UpdateLeave(surname, firstname, (leave+1));
						}
					}
					else
						cout<<"Employee is not exist"<<endl;
					break;
				}
			case 3: // Serch employee
				{
					string s_name, f_name;

					cout<<"Enter surname"<<endl;
					cin>>s_name;

					cout<<"Enter firstname"<<endl;
					cin>>f_name;

					employees[0]->SerchEmployee(s_name, f_name);

				break;
				}
			case 4: // Display employees
				{
					employees[0]->DisplayEmployee();
				break;
				}
			case 5: //Delete employee
				{
					string s_name, f_name;

					cout<<"Enter surname"<<endl;
					cin>>s_name;

					cout<<"Enter firstname"<<endl;
					cin>>f_name;

					employees[0]->DeleteEmployee(s_name, f_name);
				break;
				}
			case 6: // Exit from code
				{
					remove("database.csv");
					exit(0);
				}
		}
	}

	return 0;
}

