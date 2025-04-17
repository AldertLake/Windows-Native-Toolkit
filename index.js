import jsonfile from "jsonfile";
import moment from "moment";
import simpleGit from "simple-git";
import random from "random";

const path = "./data.json";

const makeCommits = (n) => {
  if (n === 0) return simpleGit().push();

  // Define the date range: March 10, 2025 to April 17, 2025
  const startDate = moment("2025-03-10");
  const endDate = moment("2025-04-17");
  
  // Calculate a random date within the range
  const dateRange = endDate.diff(startDate, "days");
  const randomDays = random.int(0, dateRange);
  const randomDate = startDate.clone().add(randomDays, "days").format();

  const data = {
    date: randomDate,
  };

  console.log(randomDate);

  jsonfile.writeFile(path, data, () => {
    simpleGit()
      .add([path])
      .commit("Only retards wrote commit title", { "--date": randomDate }, () => {
        makeCommits(n - 1);
      });
  });
};

makeCommits(1000);