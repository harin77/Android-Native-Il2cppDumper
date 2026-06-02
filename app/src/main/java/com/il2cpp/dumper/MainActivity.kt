package com.il2cpp.dumper

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.animation.AnimatedContentTransitionScope
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.History
import androidx.compose.material.icons.filled.Home
import androidx.compose.material.icons.filled.Info
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import android.app.Application
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.navigation.NavType
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import androidx.navigation.navArgument
import com.il2cpp.dumper.navigation.Screen
import com.il2cpp.dumper.ui.screens.AboutScreen
import com.il2cpp.dumper.ui.screens.HomeScreen
import com.il2cpp.dumper.ui.screens.JobDetailScreen
import com.il2cpp.dumper.ui.screens.JobHistoryScreen
import com.il2cpp.dumper.ui.screens.LicensesScreen
import com.il2cpp.dumper.ui.theme.Il2CppDumperTheme
import com.il2cpp.dumper.viewmodel.DumperViewModel
import com.il2cpp.dumper.viewmodel.DumperViewModelFactory
import com.il2cpp.dumper.viewmodel.JobDetailViewModel
import com.il2cpp.dumper.viewmodel.JobDetailViewModelFactory
import com.il2cpp.dumper.viewmodel.JobHistoryViewModel
import com.il2cpp.dumper.viewmodel.JobHistoryViewModelFactory

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            Il2CppDumperTheme {
                AppNavigation()
            }
        }
    }
}

private data class BottomNavItem(
    val label: String,
    val icon: ImageVector,
    val route: String
)

@Composable
fun AppNavigation() {
    val navController = rememberNavController()
    val app = LocalContext.current.applicationContext as Application
    val dumperViewModel: DumperViewModel = viewModel(factory = DumperViewModelFactory(app))
    val jobHistoryViewModel: JobHistoryViewModel = viewModel(factory = JobHistoryViewModelFactory(app))

    val bottomNavItems = listOf(
        BottomNavItem("Dump", Icons.Default.Home, Screen.Home.route),
        BottomNavItem("History", Icons.Default.History, Screen.JobHistory.route),
        BottomNavItem("About", Icons.Default.Info, Screen.About.route)
    )

    val navBackStackEntry by navController.currentBackStackEntryAsState()
    val currentRoute = navBackStackEntry?.destination?.route

    val showBottomBar = currentRoute in bottomNavItems.map { it.route }

    Scaffold(
        bottomBar = {
            if (showBottomBar) {
                NavigationBar {
                    bottomNavItems.forEach { item ->
                        NavigationBarItem(
                            icon = { Icon(item.icon, contentDescription = item.label) },
                            label = { Text(item.label) },
                            selected = currentRoute == item.route,
                            onClick = {
                                if (currentRoute != item.route) {
                                    navController.navigate(item.route) {
                                        popUpTo(Screen.Home.route) { saveState = true }
                                        launchSingleTop = true
                                        restoreState = true
                                    }
                                }
                            }
                        )
                    }
                }
            }
        }
    ) { paddingValues ->
        NavHost(
            navController = navController,
            startDestination = Screen.Home.route,
            modifier = Modifier
                .fillMaxSize()
                .padding(bottom = paddingValues.calculateBottomPadding()),
            enterTransition = {
                slideIntoContainer(AnimatedContentTransitionScope.SlideDirection.Left, tween(300)) + fadeIn(tween(300))
            },
            exitTransition = {
                slideOutOfContainer(AnimatedContentTransitionScope.SlideDirection.Left, tween(300)) + fadeOut(tween(300))
            },
            popEnterTransition = {
                slideIntoContainer(AnimatedContentTransitionScope.SlideDirection.Right, tween(300)) + fadeIn(tween(300))
            },
            popExitTransition = {
                slideOutOfContainer(AnimatedContentTransitionScope.SlideDirection.Right, tween(300)) + fadeOut(tween(300))
            }
        ) {
            composable(Screen.Home.route) {
                HomeScreen(
                    viewModel = dumperViewModel,
                    onNavigateToJob = { jobId ->
                        navController.navigate(Screen.JobDetail.createRoute(jobId))
                    }
                )
            }

            composable(Screen.JobHistory.route) {
                JobHistoryScreen(
                    viewModel = jobHistoryViewModel,
                    onJobClick = { jobId ->
                        navController.navigate(Screen.JobDetail.createRoute(jobId))
                    }
                )
            }

            composable(
                route = Screen.JobDetail.route,
                arguments = listOf(navArgument("jobId") { type = NavType.LongType })
            ) { backStackEntry ->
                val jobId = backStackEntry.arguments?.getLong("jobId") ?: return@composable
                val jobDetailViewModel: JobDetailViewModel = viewModel(
                    factory = JobDetailViewModelFactory(app, jobId),
                    key = "jobDetail_$jobId"
                )
                JobDetailScreen(
                    viewModel = jobDetailViewModel,
                    onBack = { navController.popBackStack() }
                )
            }

            composable(Screen.About.route) {
                AboutScreen(
                    onNavigateToLicenses = {
                        navController.navigate(Screen.Licenses.route)
                    }
                )
            }

            composable(Screen.Licenses.route) {
                LicensesScreen(
                    onBack = { navController.popBackStack() }
                )
            }
        }
    }
}
