final string COMPANY_NAME = "TechCorp";

class Developer {
    string name;
    final string ROLE = "Developer";
    static int totalDevelopers = 0;
    static final int MAX_PROJECTS = 5;

    public constructor(string devName) {
        name = devName;
        totalDevelopers = totalDevelopers + 1;
    }

    public static function getTeamSize(): int {
        return totalDevelopers;
    }

    public static function canTakeMoreProjects(int currentProjects): bool {
        return currentProjects < MAX_PROJECTS;
    }

    public function getInfo(): string {
        return name + " - " + ROLE;
    }
}

final string BACKEND_TECH_STACK = "C++";
int completedProjects = 0;

function assignProject(Developer dev): int {
    completedProjects = completedProjects + 1;
    return completedProjects;
}

function getTechStack(): string {
    return BACKEND_TECH_STACK;
}

final string FRONTEND_FRAMEWORK = "React";
int uiComponents = 0;

function createComponent(string componentName): int {
    uiComponents = uiComponents + 1;
    return uiComponents;
}

final int CAMPAIGN_BUDGET = 10000;
int activeCampaigns = 0;

function launchCampaign(): int {
    activeCampaigns = activeCampaigns + 1;
    return activeCampaigns;
}

// Test deep namespace access with static methods
Developer dev1 = new Developer("Alice");
Developer dev2 = new Developer("Bob");

print(Developer::getTeamSize());
print(dev1.getInfo());

int project1 = assignProject(dev1);
int project2 = assignProject(dev2);
print(project1);
print(project2);

print(getTechStack());

int comp1 = createComponent("Header");
int comp2 = createComponent("Footer");
print(comp1);
print(comp2);

int campaign = launchCampaign();
print(campaign);

// Test using directives with deeply nested namespaces
// Note: Namespaces removed, functions now global
Developer dev3 = new Developer("Charlie");
int project3 = assignProject(dev3);
print(Developer::getTeamSize());
print(project3);