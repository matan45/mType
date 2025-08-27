namespace company {
    final string COMPANY_NAME = "TechCorp";
    
    namespace department {
        namespace engineering {
            class Developer {
                string name;
                final string ROLE = "Developer";
                static int totalDevelopers = 0;
                static final int MAX_PROJECTS = 5;
                
                constructor(string devName) {
                    name = devName;
                    totalDevelopers = totalDevelopers + 1;
                }
                
                static function getTeamSize(): int {
                    return totalDevelopers;
                }
                
                static function canTakeMoreProjects(int currentProjects): bool {
                    return currentProjects < MAX_PROJECTS;
                }
                
                function getInfo(): string {
                    return name + " - " + ROLE;
                }
            }
            
            namespace backend {
                final string TECH_STACK = "C++";
                int completedProjects = 0;
                
                function assignProject(company::department::engineering::Developer dev): int {
                    completedProjects = completedProjects + 1;
                    return completedProjects;
                }
                
                function getTechStack(): string {
                    return TECH_STACK;
                }
            }
            
            namespace frontend {
                final string FRAMEWORK = "React";
                int uiComponents = 0;
                
                function createComponent(string componentName): int {
                    uiComponents = uiComponents + 1;
                    return uiComponents;
                }
            }
        }
        
        namespace marketing {
            final int CAMPAIGN_BUDGET = 10000;
            int activeCampaigns = 0;
            
            function launchCampaign(): int {
                activeCampaigns = activeCampaigns + 1;
                return activeCampaigns;
            }
        }
    }
}

// Test deep namespace access with static methods
company::department::engineering::Developer dev1 = new company::department::engineering::Developer("Alice");
company::department::engineering::Developer dev2 = new company::department::engineering::Developer("Bob");

print(company::department::engineering::Developer::getTeamSize());
print(dev1.getInfo());

int project1 = company::department::engineering::backend::assignProject(dev1);
int project2 = company::department::engineering::backend::assignProject(dev2);
print(project1);
print(project2);

print(company::department::engineering::backend::getTechStack());

int comp1 = company::department::engineering::frontend::createComponent("Header");
int comp2 = company::department::engineering::frontend::createComponent("Footer");
print(comp1);
print(comp2);

int campaign = company::department::marketing::launchCampaign();
print(campaign);

// Test using directives with deeply nested namespaces
using namespace company::department::engineering;
using namespace company::department::engineering::backend;

Developer dev3 = new Developer("Charlie");
int project3 = assignProject(dev3);
print(Developer::getTeamSize());
print(project3);