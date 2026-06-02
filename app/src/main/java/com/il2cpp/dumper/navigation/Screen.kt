package com.il2cpp.dumper.navigation

sealed class Screen(val route: String) {
    data object Home : Screen("home")
    data object JobHistory : Screen("job_history")
    data object JobDetail : Screen("job_detail/{jobId}") {
        fun createRoute(jobId: Long) = "job_detail/$jobId"
    }
    data object About : Screen("about")
    data object Licenses : Screen("licenses")
}
